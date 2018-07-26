#include "ix_indexscan.h"
#include "ix_error.h"
using namespace std;


IXIndexScan::~IXIndexScan()
{
    if (comp_) delete(comp_);
}

RC IXIndexScan::openScan(const IXIndexHandle &index, CompOp op, const void* val)
{
    op_ = op;
    val_ = val;
    index_ = const_cast<IXIndexHandle*>(&index);
    opened_ = true;
    comp_ = make_comp(index.GetAttrType(), index_->GetAttrLength());
    //这里并不会设定pCurr_和currPos_
    return 0;
}

RC IXIndexScan::getNextEntry(RID &rid)
{
    void* key = NULL;
    return getNextEntry(key, rid);
}

//
//
RC IXIndexScan::getNextEntry(void *&key_, RID &rid)
{
    switch (op_)
    {
        case EQ_OP:
            return get_eq_op(key_, rid);
            break;

        case NO_OP:
            return get_no_ne_op(key_, rid);
            break;

        case NE_OP:
            return get_no_ne_op(key_, rid);
            break;

        case LE_OP:
            return get_lt_le_op(key_, rid);
            break;

        case LT_OP:
            return get_lt_le_op(key_, rid);
            break;

        case GE_OP:
            return get_gt_ge_op(key_, rid);
            break;

        case GT_OP:
            return get_gt_ge_op(key_, rid);
            break;
    }
}

//相等比较
RC IXIndexScan::get_eq_op(void *&key_, RID &rid)
{
    if (!opened_) return IX_FNOTOPEN;
    if (eof_) return IX_EOF;
    /* 第一次调用getNextEntry, curr_和pos_都没有被初始化过 */
    if ((curr_ == NULL) && (pos_ == 0)) {
        curr_ = index_->FindLeaf(val_);
        if(curr_ == NULL)
            return IX_EOF;
    }

    while (curr_ != NULL) {
        int size = curr_->GetNumKeys();
        void* key = NULL;
        for (int i = pos_; i < size; i++)
        {
            curr_->GetKey(i, key);
            if (comp_->eval(key, op_, val_)) {
                rid = curr_->GetAddr(i);;
                key_ = key;
                pos_ = i + 1;
                return 0;
            }
        }
        curr_ = index_->FetchNode(curr_->GetRight());
        pos_ = 0;
    }
    eof_ = true;
    return IX_EOF;
}


//不等和没有比较
//直接遍历叶子节点就行了
RC IXIndexScan::get_no_ne_op(void *&key_, RID &rid)
{
    if (!opened_) return IX_FNOTOPEN;
    if (eof_) return IX_EOF;
    /* 第一次调用getNextEntry, curr_和pos_都没有被初始化过 */
    if ((curr_ == NULL) && (pos_ == 0)) {
        curr_ = index_->FetchNode(index_->FindSmallestLeaf()->GetPageRID());
    }

    while (curr_ != NULL) {
        int size = curr_->GetNumKeys();
        void* key = NULL;
        for (int i = pos_; i < size; i++)
        {
            curr_->GetKey(i, key);
            if (comp_->eval(key, op_, val_)) {
                rid = curr_->GetAddr(i);
                key_ = key;
                pos_ = i + 1;
                return 0;
            }
        }
        curr_ = index_->FetchNode(curr_->GetRight());
        pos_ = 0;
    }
    eof_ = true;
    return IX_EOF;
}
//小于和小于等于比较
//先找到叶子再向左遍历
RC IXIndexScan::get_lt_le_op(void *&key_, RID &rid)
{
    if (!opened_) return IX_FNOTOPEN;
    if (eof_) return IX_EOF;
    /* 第一次调用getNextEntry, curr_和pos_都没有被初始化过 */
    if ((curr_ == NULL) && (pos_ == 0)) {
        curr_ = index_->FindLeaf(val_);
        if(curr_ == NULL)
            return IX_EOF;
        pos_ = curr_->GetNumKeys()-1;
    }

    while (curr_ != NULL) {
        void* key = NULL;
        for (int i = pos_; i >=0 ; i--)
        {
            curr_->GetKey(i, key);
            if (comp_->eval(key, op_, val_)) {
                rid = curr_->GetAddr(i);
                key_ = key;
                pos_ = i -1;
                return 0;
            }
        }
        curr_ = index_->FetchNode(curr_->GetRight());
        pos_ = curr_->GetNumKeys()-1;
    }
    eof_ = true;
    return IX_EOF;
}

//大于和大于等于比较
RC IXIndexScan::get_gt_ge_op(void *&key_, RID &rid)
{
    if (!opened_) return IX_FNOTOPEN;
    if (eof_) return IX_EOF;
    /* 第一次调用getNextEntry, curr_和pos_都没有被初始化过 */
    if ((curr_ == NULL) && (pos_ == 0)) {
        curr_ = index_->FindLeaf(val_);
        if(curr_ == NULL)
            return IX_EOF;
    }

    while (curr_ != NULL) {
        int size = curr_->GetNumKeys();
        void* key = NULL;
        for (int i = pos_; i < size; i++)
        {
            curr_->GetKey(i, key);
            if (comp_->eval(key, op_, val_)) {
                rid = curr_->GetAddr(i);
                key_ = key;
                pos_ = i + 1;
                return 0;
            }
        }
        curr_ = index_->FetchNode(curr_->GetRight());
        pos_ = 0;
    }
    eof_ = true;
    return IX_EOF;
}


RC IXIndexScan::closeScan()
{
    if (!opened_) return IX_FNOTOPEN;
    opened_ = false;
    if (comp_ != NULL) delete comp_;
    comp_ = NULL;
    curr_ = NULL;
    pos_ = -1;
    eof_ = false;
    return 0;
}

RC IXIndexScan::rewind()
{
    if (!opened_) return IX_FNOTOPEN;
    pos_ = 0;
    curr_ = NULL;
    eof_ = false;
    return 0;
}

