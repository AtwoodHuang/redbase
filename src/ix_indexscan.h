
#ifndef IX_INDEXSCAN_H
#define IX_INDEXSCAN_H

#include "common_thing.h"
#include "rm_rid.h"
#include "comp.h"
#include "ix_indexhandle.h"
class BPlusNode;
class IXIndexHandle;

class IXIndexScan {
public:
    IXIndexScan()
            : opened_(false), eof_(false), comp_(NULL)
            , curr_(NULL), pos_(0), op_(NO_OP), val_(NULL)
    {}
    ~IXIndexScan();
public:
    RC openScan(const IXIndexHandle &index, CompOp comp, const void* val);
    RC getNextEntry(RID &rid);
    RC rewind();
    RC closeScan();
public:
    RC getNextEntry(void* &key, RID& rid);
private:
    //相等比较
    RC get_eq_op(void *&key_, RID &rid);
    //不等和没有比较
    RC get_no_ne_op(void *&key_, RID &rid);
    //小于和小于等于比较
    RC get_lt_le_op(void *&key_, RID &rid);
    //大于和大于等于比较
    RC get_gt_ge_op(void *&key_, RID &rid);
    Comp *comp_;
    IXIndexHandle* index_;
    BtreeNode* curr_;
    int pos_;
    bool opened_;
    bool eof_;
    CompOp op_;
    const void* val_;
};

#endif /* IX_INDEXSCAN_H */