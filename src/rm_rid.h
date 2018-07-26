#ifndef REDBASE_RM_RID_H
#define REDBASE_RM_RID_H

#include <iostream>
#include "common_thing.h"

// PageNum: page在文件中的编号
typedef int PageNum;

// SlotNum: record在文件中的号
typedef int SlotNum;

//
class RID {
public:
    static const PageNum NULL_PAGE = -1;
    static const SlotNum NULL_SLOT = -1;
    RID() :page(NULL_PAGE), slot(NULL_SLOT) {}
    RID(PageNum pageNum, SlotNum slotNum) :page(pageNum), slot(slotNum) {}
    ~RID(){}

    RC GetPageNum(PageNum &pageNum) const          // 返回page
    {
        pageNum = page; return 0;
    }

    RC GetSlotNum(SlotNum &slotNum) const         // 返回slot
    {
        slotNum = slot; return 0;
    }

    bool operator==(const RID & rhs) const
    {
        PageNum p;
        SlotNum s;
        rhs.GetPageNum(p);
        rhs.GetSlotNum(s);
        return (p == page && s == slot);
    }
    PageNum page;
    SlotNum slot;
};

std::ostream& operator <<(std::ostream & os, const RID& r);
#endif //REDBASE_RM_RID_H
