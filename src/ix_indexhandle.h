#ifndef REDBASE_IX_INDEXHANDLE_H
#define REDBASE_IX_INDEXHANDLE_H

#include "common_thing.h"
#include "rm_rid.h"
#include "ix_common.h"
#include "pf_filehandle.h"
#include "btree_node.h"
#include "iostream"

class IXIndexHandle {
    friend class IXManager;

public:
    IXIndexHandle();
    ~IXIndexHandle();

    RC InsertEntry(void *pData, const RID &rid);

    RC DeleteEntry(void *pData, const RID &rid);

    RC Search(void *pData, RID &rid);

    // Force index files to disk
    RC ForcePages();

    RC Open(PFFileHandle * pfh);
    RC GetFileHeader(PFPageHandle ph);
    // persist header into the first page of a file for later
    RC SetFileHeader(PFPageHandle ph) const;

    bool HdrChanged() const { return bHdrChanged; }
    int GetNumPages() const { return hdr.numPages; }
    AttrType GetAttrType() const { return hdr.attrType; }
    int GetAttrLength() const { return hdr.attrLength; }

    RC GetNewPage(PageNum& pageNum);
    RC DisposePage(const PageNum& pageNum);

    RC IsValid() const;

    BtreeNode* FindLeaf(const void *pData);
    BtreeNode* FindSmallestLeaf();
    BtreeNode* FindLargestLeaf();

    BtreeNode* FetchNode(RID r) const;
    BtreeNode* FetchNode(PageNum p) const;


    // get/set height
    int GetHeight() const;
    void SetHeight(const int&);

    BtreeNode* GetRoot() const;
    bool bFileOpen;
    bool bHdrChanged;
private:

    RC GetThisPage(PageNum p, PFPageHandle& ph) const;

    IXFileHdr hdr;

    PFFileHandle * pfHandle;

    BtreeNode * root; // 根节点
    BtreeNode ** path; // 从根到叶子的路径
    int* pathP; // 路径上对应的pos


    void * treeLargest; //最大key
};

#endif //REDBASE_IX_INDEXHANDLE_H
