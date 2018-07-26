#ifndef REDBASE_BTREE_NODE_H
#define REDBASE_BTREE_NODE_H


#include "common_thing.h"
#include "pf_filehandle.h"
#include "rm_rid.h"
#include "iostream"

class BtreeNode {
public:
    BtreeNode(AttrType attrType, int attrLength,
              PFPageHandle& ph, bool newPage = true,
              int pageSize = PF_PAGE_SIZE);
    RC ResetBtreeNode(PFPageHandle& ph, const BtreeNode& rhs);
    ~BtreeNode();
    int Destroy();

    friend class IX_IndexHandle;
    RC IsValid() const;
    int GetMaxKeys() const;

    int GetNumKeys();
    int SetNumKeys(int newNumKeys);
    int GetLeft();
    int SetLeft(PageNum p);
    int GetRight();
    int SetRight(PageNum p);


    RC GetKey(int pos, void* &key) const;
    int SetKey(int pos, const void* newkey);
    int CopyKey(int pos, void* toKey) const;


    int Insert(const void* newkey, const RID& newrid);

    int Remove(const void* newkey, int kpos = -1);

    int FindKey(const void* &key, const RID& r = RID(-1,-1)) const;

    RID FindAddr(const void* &key) const;

    RID GetAddr(const int pos) const;


    int FindKeyPosition(const void* &key) const;
    RID FindAddrAtPosition(const void* &key) const;

    RC Split(BtreeNode* rhs);
    RC Merge(BtreeNode* rhs);


    RID GetPageRID() const;
    void SetPageRID(const RID&);

    int CmpKey(const void * k1, const void * k2) const;
    bool isSorted() const;
    void* LargestKey() const;
    void Print(std::ostream & os);

private:

    char * keys;
    RID * rids;
    int numKeys;//放了多少个
    int attrLength;
    AttrType attrType;
    int maxnum;//能放多少个记录
    RID pageRID; //该b+树节点所在页
};


#endif //REDBASE_BTREE_NODE_H
