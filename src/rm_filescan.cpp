//
// rm_filescan.cpp
// 类RMFileScan的实现
//

#include <iostream>
#include "common_thing.h"
#include "rm_filescan.h"
#include "pf_pagehandle.h"
#include "iostream"
using namespace std;

RC RMFileScan::openScan(const RMFileHandle& file, AttrType type, int len, int attroffset, CompOp op, const void* val)
{
    rmfile_ = const_cast<RMFileHandle*>(&file);
    if (val != NULL) {
        /* 做一些检查 */
        if ((len >= PF_PAGE_SIZE - sizeof(RID)) || (len <= 0)) return RM_RECSIZEMISMATCH;
        if (type == STRING && (len <= 0 || len > MAXSTRINGLEN))
            return RM_FCREATEFAIL;
    }
    comp_ = make_comp(type, len);
    op_ = op;
    val_ = val;
    offset_ = attroffset; /* 记录下属性的偏移量 */
    return 0;
}

//
// rewind - 从新开始遍历
//
RC RMFileScan::rewind()
{
    curr_ = RID(1, -1);//第0页是RM文件头
    return 0;
}


//
// getNextRcd - 用于获取下一条记录
//在一个pffile里搜索记录
RC RMFileScan::getNextRcd(RMRecord &rcd)
{
    PFPageHandle ph;
    RMPageHdr pHdr(rmfile_->capacity_);
    RC rc;

    for( int j = curr_.page; j < rmfile_->pagesSize(); j++) {//遍历文件中的page
        if((rc = rmfile_->pffile_->getthisPage(j, ph))
           || (rc = rmfile_->pffile_->unpinpage(j)))
            return rc;

        if((rc = rmfile_->GetPageHeader(ph, pHdr)))
            return rc;
        bitmap b(pHdr.freeSlotMap, rmfile_->capacity_);

        int i = -1;
        if(curr_.page == j)
            i = curr_.slot+1;//下一个slot
        else
            i = 0;
        for (; i < rmfile_->capacity_; i++)
        {
            if (b.test(i)) {//这个slot已被使用
                curr_ = RID(j, i);
                rmfile_->getRcd(curr_, rcd);
                char * pData = NULL;
                rcd.GetData(pData);
                if(comp_->eval(pData + offset_, op_, val_))
                {
                    curr_.page = j;
                    curr_.slot = i;
                    return 0;
                }
                else
                {

                }
            }
        }
    }
    return RM_EOF;
}

RC RMFileScan::closeScan()
{
    if (comp_ != NULL) delete comp_;
    curr_ = RID(1, -1);
    return 0;
}