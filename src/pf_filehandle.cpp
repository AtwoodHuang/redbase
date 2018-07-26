#include <zconf.h>
#include "pf_filehandle.h"
#include "pf_pagehandle.h"
//拷贝构造函数
PFFileHandle::PFFileHandle(const PFFileHandle &fileHandle)
{
    this->buff  = fileHandle.buff;
    this->hdr   = fileHandle.hdr;
    this->opened   = fileHandle.opened;
    this->changed = fileHandle.changed;
    this->fd  = fileHandle.fd;
}
//赋值运算符
PFFileHandle& PFFileHandle::operator=(const PFFileHandle &fileHandle)
{
    if (this != &fileHandle)
    {
        this->buff  = fileHandle.buff;
        this->hdr   = fileHandle.hdr;
        this->opened  = fileHandle.opened;
        this->changed = fileHandle.changed;
        this->fd   = fileHandle.fd;
    }
    return (*this);
}
//
// getFirstPage - 获取第一个页面,将内容填入PFPageHandle这样一个实例中
//
RC PFFileHandle::getfirstPage(PFPageHandle &page) const
{
    return getnextPage((int)-1, page);
}

//
// getLastPage - 获取最后一个页面,将内容填充如PFPageHandle实例中
//
RC PFFileHandle::getlastPage(PFPageHandle &page) const
{
    return getprevPage(hdr.size, page);
}

//
// getNextPage - 获取当前页面的下一个页面,并将内容填充如PFPageHandle的一个实例中
//
RC PFFileHandle::getnextPage(int curr, PFPageHandle &page) const
{
    RC rc;
    if (!opened)
        return PF_CLOSEDFILE;
    if (curr < -1 || curr >= hdr.size)
        return PF_INVALIDPAGE;
    // 扫描文件直到一个有效的使用过的页面被找到
    for (curr++; curr < hdr.size; curr++)
    {
        if (!(rc = getthisPage(curr, page)))
            return 0;
        if (rc != PF_INVALIDPAGE)
            return rc;
    }
    return PF_EOF;
}

//
// getPrevPage - 获取前一个页面的信息,并将内容填入PFPageHandle的一个实例中
//
RC PFFileHandle::getprevPage(int curr, PFPageHandle &page) const
{
    RC rc;
    if (!opened)
        return PF_CLOSEDFILE;
    if (curr <= 0 || curr >= hdr.size)
        return PF_INVALIDPAGE;

    // 扫描文件，直到找到一个有效的页,所谓的有效的页,指的是已使用过的页
    for (curr--; curr >= 0; curr--)
    {
        if (!(rc = getthisPage(curr, page)))
            return 0;

        // If unexpected error, return it
        if (rc != PF_INVALIDPAGE)
            return rc;
    }
    return PF_EOF;
}

RC PFFileHandle::pin(int num)
{
    if (!opened)
        return PF_CLOSEDFILE;
    if (num < 0 || num >= hdr.size)
        return PF_INVALIDPAGE;
    return buff->pin(fd, num);
}

//
// getPage - 获取文件中指定的页, 并将内容填充入一个PFPageHandle的一个实体
//
RC PFFileHandle::getthisPage(int num, PFPageHandle &page) const
{

    if (!opened)
        return PF_CLOSEDFILE;
    if (num < 0 || num >= hdr.size)
        return PF_INVALIDPAGE;
    char* addr;
    buff->getPage(fd, num, addr);
    PF_Page_Head *hdr = (PF_Page_Head *)addr;
    // 如果页面不空闲，那么让page指向这个页面
    if (hdr->free == PF_PAGE_USED)
    {
        page.num = num;
        // 注意到这里的data,指向的并不是页的首部,而是首部后面4个字节处
        page.addr = addr + sizeof(PF_Page_Head);
        return 0;
    }
    // 页面空闲（count为0后且未dirty会被写入磁盘）
    unpinpage(num);
    return PF_INVALIDPAGE;
}

//
// allocatePage - 在文件中分配一个新的页面
//
RC PFFileHandle::allocPage(PFPageHandle &page)
{
    RC rc;		// 返回码
    int num;
    char* addr;

    if (!opened)
        return PF_CLOSEDFILE;

    if (hdr.free != PF_PAGE_LIST_END)
    { // 仍然存在空闲页面,取出一块
        num = hdr.free;//下一个空闲页面
        if (rc = buff->getPage(fd, num, addr))//从buff里取如果buff没有会从磁盘里取出来
            return rc;
        hdr.free = ((PF_Page_Head *)addr)->free; // 空闲数目减1
    }
    else
    { // 空闲链表为空
        num = hdr.size; //增加一页
        // 分配一个新的页面
        if (rc = buff->allocPage(fd, num, addr))
            return rc;
        hdr.size++;
    }
    changed = true; // 文件发生了变动
    // 将这个页面标记为USED
    ((PF_Page_Head *)addr)->free = PF_PAGE_USED;
    memset(addr + sizeof(PF_Page_Head), 0, PF_PAGE_SIZE);
    // 将页面标记为脏
    markDirty(num);
    // 将页面的信息填入pph中
    page.num = num;
    page.addr = addr + sizeof(PF_Page_Head); // 指向实际的数据
    return 0;
}

//
// disposePage - 销毁一个页面,需要注意的是,PFPageHandle实例指向的对象
// 应该不再被使用了，在调用了这个函数之后。
//
RC PFFileHandle::disposePage(int num)
{
    char* addr;
    if (num < 0 || num >= hdr.size)
        return PF_INVALIDPAGE;
    // 常规性检查，文件必须要打开，然后页面必须有效
    buff->getPage(fd, num, addr);
    PF_Page_Head *newhdr = (PF_Page_Head *)addr;
    // 页面必须有效,free == PF_PAGE_USED表示页面正在被使用
    if (newhdr->free != PF_PAGE_USED)
    {
        unpinpage(num);
        return PF_PAGEFREE;
    }
    newhdr->free = hdr.free; // 将销毁的页面放入链表中,这么做是为了避免产生空洞
    hdr.free = num;
    changed = true;
    markDirty(num);
    unpinpage(num);
    return 0;
}

//
// markDirty - 将一个页面标记为脏页面,表示该页面已被更改
//
RC PFFileHandle::markDirty(int num) const
{
    if (!opened)
        return PF_CLOSEDFILE;
    if (num < 0 || num >= hdr.size)
        return PF_INVALIDPAGE;
    return buff->markDirty(fd, num);
}

//
// flushPages - 将所有的脏的页面回写到磁盘中去.
//
RC PFFileHandle::flush()
{
    if (!opened)
        return PF_CLOSEDFILE;
    int n;
    // 如果头部被修改了，那么将其写回到磁盘中
    if (changed)
    {
        lseek(fd, 0, SEEK_SET);
        // 写头部
        n = write(fd, &hdr, sizeof(PF_File_Head));
        if (n != sizeof(PF_File_Head))
            return PF_HDRWRITE;
        changed = false;
    }
    return buff->flush(fd); // 将内容刷到磁盘中
}


//
// unpin - 如果有必要的话，这个页面将会被写回到磁盘中，即不再需要这个页面。
//
RC PFFileHandle::unpinpage(int num) const
{
    if (!opened)
        return PF_CLOSEDFILE;
    if (num < 0 || num >= hdr.size)
        return PF_INVALIDPAGE;
    return buff->unpin(fd, num);
}

RC PFFileHandle::forcePages(int num )
{
    if (!opened)
        return PF_CLOSEDFILE;
    if (changed)
    {
        lseek(fd, 0, SEEK_SET);
        int size = sizeof(PF_File_Head);
        int n = write(fd, &hdr, size);
        if (n != size)
            return PF_HDRWRITE;
        changed = false;
    }
    return buff->forcePages(fd, num);
}

void PFFileHandle::clearFilePages()
{
    buff->clearFilePages(fd);
}

