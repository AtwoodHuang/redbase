#include "rm_filehandle.h"
#include "rm_error.h"
#include <cmath>


//
// CalcSlotsSize - 计算一个数据块最多所支持的存储的记录的条数
//
unsigned int static CalcSlotsCapacity(unsigned int len)
{
    unsigned int remain = PF_PAGE_SIZE - sizeof(RMPageHdr); // 剩余可用的字节数目
    // floor - 向下取整
    // 每一条记录都需要1个bit,也就是1/8字节来表示是否已经记录了数据
    unsigned int slots = floor((1.0 * remain) / (len + 1 / 8));
    unsigned int hdr_size = sizeof(RMPageHdr) + bitmap(slots).numChars();
    // 接下来需要不断调整
    while ((slots * len + hdr_size) > PF_PAGE_SIZE)
    {
        slots--;
        hdr_size = sizeof(RMPageHdr) + bitmap(slots).numChars();
    }
    return slots;
}

//把pfpage里的rmfileheadr读进来
RC RMFileHandle::GetFileHeader(PFPageHandle ph)
{
    char * buf;
    RC rc = ph.GetData(buf);
    memcpy(&rmhdr_, buf, sizeof(RMFileHdr));
    return rc;
}

//把pfpage里的rmpage头读到phdr里
RC RMFileHandle::GetPageHeader(PFPageHandle ph, RMPageHdr& pHdr) const
{
    char * buf;
    RC rc = ph.GetData(buf);
    pHdr.from_buf(buf);
    return rc;
}

//把rmfile头放到ph里
RC RMFileHandle::SetFileHeader(PFPageHandle ph) const
{
    char * buf;
    RC rc = ph.GetData(buf);
    memcpy(buf, &rmhdr_, sizeof(RMFileHdr));
    return rc;
}
//把phdr里的rmpage头放到ph里
RC RMFileHandle::SetPageHeader(PFPageHandle ph, const RMPageHdr& pHdr)
{
    char * buf;
    RC rc;
    if((rc = ph.GetData(buf)))
        return rc;
    pHdr.to_buf(buf);
    return 0;
}

//根据slot找到slot的指针
RC RMFileHandle::GetSlotPointer(PFPageHandle ph, SlotNum s, char *& pData) const
{
    RC rc = ph.GetData(pData);
    if (rc >= 0 ) {
        pData = pData + (RMPageHdr(capacity_).size()); //ptr初始指向RMpage开始处，先跳过rmpage头
        pData = pData + s * rcdlen_;//slot从0开始，
    }

    return rc;
}


RC RMFileHandle::open(PFFileHandle* file)
{
    pffile_ = new PFFileHandle;
    *pffile_ = *file;
    PFPageHandle page;

    pffile_->getthisPage(0,page); //第0页是rm file头
    GetFileHeader(page);//把rm头放进结构题内
    pffile_->unpinpage(0);//每次用完后都要unpin一次
    rcdlen_ = rmhdr_.rcdlen; //每条记录长度
    capacity_ = CalcSlotsCapacity(rcdlen_);//有多少个slot
    changed_ = true;
    return 0;
}

RMFileHandle::~RMFileHandle()
{
    if (pffile_ != NULL)
    {
        delete pffile_;
    }
}


//
// isValidPage - 判断是否为有效的页面
//
bool RMFileHandle::isValidPage(PageNum num) const
{
    return (num >= 0) && (num < rmhdr_.size);
}

bool RMFileHandle::isValidRID(const RID& rid) const
{
    return isValidPage(rid.page) && rid.slot >= 0;
}


//
// insertRcd - 插入一条记录
//
RC RMFileHandle::insertRcd(const char* addr, RID &rid)
{
    if (addr == NULL)
        return RM_NULLRECORD; // 指向的内容为空
    PFPageHandle page;
    PageNum num;
    SlotNum slot;
    // 从一个拥有空闲的pos的空闲页面中获取一个空闲的pos
    nextFreeSlot(page, num, slot);
    //把rm头部放到hdr中
    RMPageHdr hdr(capacity_);
    GetPageHeader(page, hdr);

    bitmap b(hdr.freeSlotMap, capacity_);
    //找到slot的指针
    char* pslot;
    GetSlotPointer(page, slot, pslot);


    rid = RID(num, slot);//生成record
    memcpy(pslot, addr, rcdlen_);//把rcdlen长度的数据插到相应的位置上
    b.set(slot); //设为1,表示已使用
    hdr.numFreeSlots--;
    if(hdr.numFreeSlots == 0)//空闲slot已用完
    {
        rmhdr_.free = hdr.free;//指向下一个空闲
        hdr.free = RM_PAGE_FULLY_USED;
    }

    b.to_char_buf(hdr.freeSlotMap, b.numChars());//把bitmap内容放回hdr
    SetPageHeader(page, hdr);//hdr内容放回pfpage
    pffile_->markDirty(num); //该页内容已经改变
    return 0;
}

//
//  updateRcd 更新一条记录, rcd中记录了记录在磁盘中的位置
//
RC RMFileHandle::updateRcd(const RMRecord &rcd)
{
    RID rid;
    rcd.GetRid(rid);
    if (!isValidRID(rid))
        return RM_BAD_RID;
    PageNum num = rid.page;
    SlotNum pos = rid.slot;
    if(!isValidRID(rid))
        return RM_BAD_RID;
    PFPageHandle page;

    RMPageHdr phdr(capacity_);
    //取出该页
    pffile_->getthisPage(num, page);
    GetPageHeader(page, phdr); //把rmpage头放到phdr里

    bitmap b(phdr.freeSlotMap, capacity_);

    if(!b.test(pos)) //还没使用，不能更新
        return RM_NORECATRID;
    char* pdata;
    rcd.GetData(pdata);

    //找到slot的指针
    char* pslot;
    GetSlotPointer(page, pos, pslot);
    //将data写入slot处
    memcpy(pslot, pdata, rcdlen_);
    pffile_->markDirty(num);//page以改变
    pffile_->unpinpage(num);//前面调过getthispage

    return 0;
}

RC RMFileHandle::deleteRcd(const RID &rid)
{
    if (!isValidRID(rid))
        return RM_BAD_RID;
    PageNum num = rid.page;
    SlotNum pos = rid.slot;
    PFPageHandle page;
    RMPageHdr phdr(capacity_);

    pffile_->getthisPage(num, page);//取出该页
    GetPageHeader(page, phdr);//取出rmpage头

    bitmap b(phdr.freeSlotMap, capacity_);

    if(!b.test(pos)) //还没用不能删
        return RM_NORECATRID;
    b.reset(pos); //已经空闲

    //有空闲位置了，加入freelist
    if(phdr.numFreeSlots == 0)
    {
        phdr.free = rmhdr_.free;
        rmhdr_.free = num;
    }

    phdr.numFreeSlots++;
    b.to_char_buf(phdr.freeSlotMap, b.numChars());//把bitmap内容放回hdr
    SetPageHeader(page, phdr);//hdr内容放回pfpage
    pffile_->markDirty(num);//page以改变
    pffile_->unpinpage(num);//前面调过getthispage
    //请注意，这里rmpage头并没有改变
    return 0;
}


//
// getRcd - 获取一条记录
//
RC RMFileHandle::getRcd(const RID &rid, RMRecord &rcd)
{
    if(!this->isValidRID(rid))
        return RM_BAD_RID;
    PageNum p = rid.page;
    SlotNum s = rid.slot;

    RC rc = 0;
    PFPageHandle ph;
    RMPageHdr pHdr(capacity_);
    pffile_->getthisPage(p, ph);
    GetPageHeader(ph, pHdr);

    bitmap b(pHdr.freeSlotMap, capacity_);

    if(!b.test(s)) //还没用
        return RM_NORECATRID;

    char * pData = NULL;
    if(RC rc = this->GetSlotPointer(ph, s, pData))
        return rc;

    rcd.set(pData, rmhdr_.rcdlen, rid);//读入记录
    pffile_->unpinpage(p);
    return 0;
}

RC RMFileHandle::forcePages(PageNum num)
{
    if (!isValidPage(num) && num != -1)
        return RM_BAD_RID;
    return pffile_->forcePages(num);
}


// getNextFreeSlot - 获取下一个空闲的pos,
// 请注意，在这里只要一个page里还有slot没用完，它就是空闲页面
bool RMFileHandle::nextFreeSlot(PFPageHandle& page, PageNum &num, SlotNum & slot)
{
//首先找到空闲的页面
    if (rmhdr_.free > 0) { // 仍然有空闲的页面,第0页是rm文件头
        pffile_->getthisPage(rmhdr_.free, page);
        num = rmhdr_.free;
        pffile_->unpinpage(rmhdr_.free);
    }
    else { // 需要重新分配页面
        char* pdata;
        pffile_->allocPage(page);
        page.GetData(pdata);
        page.GetPageNum(num);
        RMPageHdr phdr(capacity_);
        phdr.free = RM_PAGE_LIST_END;
        bitmap b(capacity_);
        b.reset();//初始所有slot都可用
        //把bitmap放到rmpage头里
        b.to_char_buf(phdr.freeSlotMap, b.numChars());
        phdr.to_buf(pdata);//把rmpage头放到page里
        pffile_->unpinpage(num);
        pffile_->markDirty(num);//page内容已改变
        //把空闲页放到list里
        rmhdr_.free = num;
        rmhdr_.size++;
        changed_ = true;//文件已改变
    }


    //再找空闲的slot
    RMPageHdr phdr2(capacity_);
    GetPageHeader(page, phdr2);
    bitmap b(phdr2.freeSlotMap, capacity_);

    for (int i = 0; i < capacity_; i++)
    {
        if (!b.test(i)) //空闲位
        {
            slot = i;
            return true;
        }
    }
    // 没空闲slot
    return false; //意外错误
}