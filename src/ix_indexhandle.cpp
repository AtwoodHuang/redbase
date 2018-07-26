#include "ix_indexhandle.h"
#include "ix_error.h"

IXIndexHandle::IXIndexHandle()
        :bFileOpen(false), pfHandle(NULL), bHdrChanged(false)
{
    root = NULL;
    path = NULL;
    pathP = NULL;
    treeLargest = NULL;
    hdr.height = 0;
}

IXIndexHandle::~IXIndexHandle()
{
    if(root != NULL && pfHandle != NULL) {
        //rootpage常驻内存，最后要unpin一下
        pfHandle->unpinpage(hdr.rootPage);
        delete root;
        root = NULL;
    }
    if(pathP != NULL) {
        delete [] pathP;
        pathP = NULL;
    }
    if(path != NULL) {
        // path[0] is root
        for (int i = 1; i < hdr.height; i++)
            if(path[i] != NULL) {
                if(pfHandle != NULL)
                    pfHandle->unpinpage(path[i]->GetPageRID().page);
                // delete path[i]; - better leak than crash
            }
        delete [] path;
        path = NULL;
    }
    if(pfHandle != NULL) {
        delete pfHandle;
        pfHandle = NULL;
    }
    if(treeLargest != NULL) {
        delete [] (char*) treeLargest;
        treeLargest = NULL;
    }
    delete pfHandle;
}

//插入新的key和rid
RC IXIndexHandle::InsertEntry(void *pData, const RID& rid)
{
    RC invalid = IsValid();
    if(invalid) return invalid;
    if(pData == NULL)
        return IX_BADKEY;

    bool newLargest = false;
    void * prevKey = NULL;
    int level = hdr.height - 1;
    BtreeNode* node = FindLeaf(pData);//找到应插的叶子节点
    pfHandle->markDirty(node->GetPageRID().page);
    BtreeNode* newNode = NULL;

    int pos2 = node->FindKey((const void*&)pData, rid);//在叶子节点中找是否已存在节点
    if((pos2 != -1))//
        return IX_ENTRYEXISTS;

    // 最大key是否改变
    if (node->GetNumKeys() == 0 || node->CmpKey(pData, treeLargest) > 0)
    {
        newLargest = true;
        prevKey = treeLargest;
    }
    //插到叶子里
    int result = node->Insert(pData, rid);

    //替换路径上的最大key
    if(newLargest)
    {
        for(int i=0; i < hdr.height-1; i++)
        {
            int pos = path[i]->FindKey((const void *&)prevKey);
            if(pos != -1)
                path[i]->SetKey(pos, pData);
            else {
                return IX_BADKEY;
            }
        }
        // 更新treelagest
        memcpy(treeLargest, pData, hdr.attrLength);
    }


    //插入失败，空间不足
    void * failedKey = pData;
    RID failedRid = rid;
    while(result == -1)
    {
        // 新建一个b+树点
        PFPageHandle ph;
        PageNum p;
        RC rc = GetNewPage(p);
        if (rc != 0)
            return rc;
        rc = GetThisPage(p, ph);
        if (rc != 0)
            return rc;

        newNode = new BtreeNode(hdr.attrType, hdr.attrLength, ph, true, hdr.pageSize);
        //分裂到新节点里
        rc = node->Split(newNode);
        if (rc != 0)
            return IX_PF;
        //处理split没考虑到的情况，newnode右边的左边连到newnode上
        BtreeNode * currRight = FetchNode(newNode->GetRight());
        if(currRight != NULL)
        {
            currRight->SetLeft(newNode->GetPageRID().page);
            delete currRight;
        }

        BtreeNode * nodeInsertedInto = NULL;

        //把新索引插进去
        if(node->CmpKey(pData, node->LargestKey()) >= 0) {
            newNode->Insert(failedKey, failedRid);
            nodeInsertedInto = newNode;
        }
        else
        {
            node->Insert(failedKey, failedRid);
            nodeInsertedInto = node;
        }

        //进入上一层
        level--;
        if(level < 0) break; //到根了
        // 找到在该层非叶子节点中索引位置
        int posAtParent = pathP[level];


        BtreeNode * parent = path[level];
        //删掉旧的索引和相应的rid
        parent->Remove(NULL, posAtParent);
        //插入上一层节点中
        result = parent->Insert((const void*)node->LargestKey(), node->GetPageRID());
        //因为分裂了，所以新结点也要插入
        result = parent->Insert(newNode->LargestKey(), newNode->GetPageRID());

        //要返回上一层
        node = parent;
        //parent是先删后插的，而newnode后插进去，因此只有可能newnode插失败了
        failedKey = newNode->LargestKey();
        failedRid = newNode->GetPageRID();
        //虽然delete了，但数据已经放在page里了
        delete newNode;
        newNode = NULL;
    }

    if(level >= 0) {
        return 0;
    } else {
        //树的高度会增加
        pfHandle->unpinpage(hdr.rootPage);

        //新建根节点
        PFPageHandle ph;
        PageNum p;
        int rc;
        rc = GetNewPage(p);
        if (rc != 0) return IX_PF;
        rc = GetThisPage(p, ph);
        if (rc != 0) return IX_PF;

        root = new BtreeNode(hdr.attrType, hdr.attrLength, ph, true, hdr.pageSize);
        root->Insert(node->LargestKey(), node->GetPageRID());
        root->Insert(newNode->LargestKey(), newNode->GetPageRID());

        hdr.rootPage = root->GetPageRID().page;
        //根page一直保持在内存区
        pfHandle->pin(hdr.rootPage);

        if(newNode != NULL) {
            delete newNode;
            newNode = NULL;
        }
        SetHeight(hdr.height+1);
        return 0;
    }
}


//删除,没有合并操作
RC IXIndexHandle::DeleteEntry(void *pData, const RID& rid)
{
    if(pData == NULL)
        return IX_BADKEY;
    RC invalid = IsValid(); if(invalid) return invalid;

    bool nodeLargest = false;

    BtreeNode* node = FindLeaf(pData);


    int pos = node->FindKey((const void*&)pData, rid);
    if(pos == -1)
    {
        int p = node->FindKey((const void*&)pData, RID(-1,-1));
        return IX_NOSUCHENTRY;
    }
    //删掉的是最大的
    else if(pos == node->GetNumKeys()-1)
        nodeLargest = true;


    if (nodeLargest) {
        //从叶子节点上一层开始
        for(int i = hdr.height-2; i >= 0; i--) {
            //在该层中找到删掉的点，如果pos最大，用下层最大点把它换掉
            int pos = path[i]->FindKey((const void *&)pData);
            if(pos != -1) {

                void * leftKey = NULL;
                //下层最大的点
                leftKey = path[i+1]->LargestKey();
                //删掉的key和下层最大的key一样，再换掉就不对了
                if(node->CmpKey(pData, leftKey) == 0) {
                    //下层第二大的key是否存在
                    int pos2 = path[i+1]->GetNumKeys()-2;
                    if(pos2 < 0) {
                        continue;
                    }
                    //leftkey变为下层第二大的key
                    path[i+1]->GetKey(path[i+1]->GetNumKeys()-2, leftKey);
                }
                //如果pos最大，用leftkey把它换掉，但是如果是叶子上一层直接换掉就行
                if((i == hdr.height-2) || (pos == path[i]->GetNumKeys()-1))
                    path[i]->SetKey(pos, leftKey);
            }
        }
    }
    //删掉该点
    int result = node->Remove(pData, pos);
    pfHandle->markDirty(node->GetPageRID().page);

    //叶子
    int level = hdr.height - 1;
    //删完为空
    while (result == -1) {
        level--;
        if(level < 0) break;
        //删掉指向node的索引
        int posAtParent = pathP[level];
        BtreeNode * parent = path[level];
        result = parent->Remove(NULL, posAtParent);
        //只剩根节点
        if(level == 0 && parent->GetNumKeys() == 1 && result == 0)
            result = -1;

        //和左右连上，旁边没有是page就是-1
        BtreeNode* left = FetchNode(node->GetLeft());
        BtreeNode* right = FetchNode(node->GetRight());
        if(left != NULL) {
            if(right != NULL)
                left->SetRight(right->GetPageRID().page);
            else
                left->SetRight(-1);
        }
        if(right != NULL) {
            if(left != NULL)
                right->SetLeft(left->GetPageRID().page);
            else
                right->SetLeft(-1);
        }
        if(right != NULL)
            delete right;
        if(left != NULL)
            delete left;
        //node清空
        node->Destroy();
        //，删掉该页
        RC rc = DisposePage(node->GetPageRID().page);
        if (rc < 0)
            return IX_PF;
        node = parent;
    }


    if(level >= 0) {
        return 0;
    } else {
        //删到根了
        if(hdr.height == 1)
        {
            //根变为叶子
            return 0;
        }

        //删光了，再第0个pos上新建一个root
        root = FetchNode(root->GetAddr(0));
        hdr.rootPage = root->GetPageRID().page;
        PFPageHandle rootph;
        RC rc = pfHandle->getthisPage(hdr.rootPage, rootph);
        if (rc != 0) return rc;
        //清楚点，销毁pfpage
        node->Destroy();
        rc = DisposePage(node->GetPageRID().page);
        if (rc < 0)
            return IX_PF;

        SetHeight(hdr.height-1);
        return 0;
    }
}


//取出相应的一页（做了markdirty和unpin）
RC IXIndexHandle::GetThisPage(PageNum p, PFPageHandle& ph) const {
    RC rc = pfHandle->getthisPage(p, ph);
    if (rc != 0)
        return rc;
    //把markdirty和unpin在这里加上，以免后面麻烦
    pfHandle->markDirty(p);
    pfHandle->unpinpage(p);
    return 0;
}

//从文件中建立一个b+树
RC IXIndexHandle::Open(PFFileHandle * pfh)
{
    bFileOpen = true;
    pfHandle = new PFFileHandle;
    *pfHandle = *pfh ;

    PFPageHandle ph;
    //0页放文件头
    GetThisPage(0, ph);

    this->GetFileHeader(ph);
    PFPageHandle rootph;

    bool newPage = true;
    if (hdr.height > 0) //文件中已存在b+树
    {
        SetHeight(hdr.height);
        newPage = false;

        RC rc = GetThisPage(hdr.rootPage, rootph);
        if (rc != 0) return rc;

    } else {//没有b+树，做额外的初始化工作
        PageNum p;
        RC rc = GetNewPage(p);
        if (rc != 0) return rc;
        rc = GetThisPage(p, rootph);
        if (rc != 0) return rc;
        hdr.rootPage = p;
        SetHeight(1);

    }
    //rootpage 常驻内存区
    pfHandle->pin(hdr.rootPage);

    root = new BtreeNode(hdr.attrType, hdr.attrLength, rootph, newPage, hdr.pageSize);
    path[0] = root;
    hdr.maxnum = root->GetMaxKeys();
    bHdrChanged = true;
    RC invalid = IsValid(); if(invalid) return invalid;
    treeLargest = (void*) new char[hdr.attrLength]();//初始最大为0
    if(!newPage) {
        BtreeNode * node = FindLargestLeaf();
        if(node->GetNumKeys() > 0)
            node->CopyKey(node->GetNumKeys()-1, treeLargest);
    }
    return 0;
}

//从文件中把头读出来
RC IXIndexHandle::GetFileHeader(PFPageHandle ph)
{
    char * buf;
    RC rc = ph.GetData(buf);
    if (rc != 0)
        return rc;
    memcpy(&hdr, buf, sizeof(hdr));
    return 0;
}

RC IXIndexHandle::SetFileHeader(PFPageHandle ph) const
{
    char * buf;
    RC rc = ph.GetData(buf);
    if (rc != 0)
        return rc;
    memcpy(buf, &hdr, sizeof(hdr));
    return 0;
}


RC IXIndexHandle::ForcePages ()
{
    RC invalid = IsValid(); if(invalid) return invalid;
    return pfHandle->forcePages(-1);
}


RC IXIndexHandle::IsValid () const
{
    bool ret = true;
    ret = ret && (pfHandle != NULL);
    if(hdr.height > 0) {
        ret = ret && (hdr.rootPage > 0); // 0 is for file header
        ret = ret && (hdr.numPages >= hdr.height + 1);
        ret = ret && (root != NULL);
        ret = ret && (path != NULL);
    }
    return ret ? 0 : IX_BADIXPAGE;
}

//新建一页
RC IXIndexHandle::GetNewPage(PageNum& pageNum)
{
    RC invalid = IsValid(); if(invalid) return invalid;
    PFPageHandle ph;

    RC rc;
    if ((rc = pfHandle->allocPage(ph)) || (rc = ph.GetPageNum(pageNum)))
        return(rc);

    //pfHandle->unpinpage(pageNum);
    pfHandle->markDirty(pageNum);
    hdr.numPages++;
    bHdrChanged = true;
    return 0;
}

//销毁一页
RC IXIndexHandle::DisposePage(const PageNum& pageNum)
{
    RC invalid = IsValid(); if(invalid) return invalid;

    RC rc;
    if ((rc = pfHandle->disposePage(pageNum)))
        return(rc);

    hdr.numPages--;
    bHdrChanged = true;
    return 0;
}

//找最小的叶子
BtreeNode* IXIndexHandle::FindSmallestLeaf()
{
    RC invalid = IsValid(); if(invalid) return NULL;
    if (root == NULL) return NULL;
    RID addr;
    if(hdr.height == 1) {
        path[0] = root;
        return root;
    }

    for (int i = 1; i < hdr.height; i++)
    {
        RID r = path[i-1]->GetAddr(0);
        if(r.page == -1) {
            return NULL;
        }
        //先删掉原来的
        if(path[i] != NULL) {
            pfHandle->unpinpage(path[i]->GetPageRID().page);
            delete path[i];
            path[i] = NULL;
        }
        path[i] = FetchNode(r);;
        //pin
        pfHandle->pin(path[i]->GetPageRID().page);
        pathP[i-1] = 0;
    }
    return path[hdr.height-1];
}

//最大
BtreeNode* IXIndexHandle::FindLargestLeaf()
{
    RC invalid = IsValid(); if(invalid) return NULL;
    if (root == NULL) return NULL;
    RID addr;
    if(hdr.height == 1) {
        path[0] = root;
        return root;
    }

    for (int i = 1; i < hdr.height; i++)
    {
        RID r = path[i-1]->GetAddr(path[i-1]->GetNumKeys() - 1);
        if(r.page == -1) {
            return NULL;
        }
        //先删掉原来的
        if(path[i] != NULL) {
            RC rc = pfHandle->unpinpage(path[i]->GetPageRID().page);
            if (rc < 0) return NULL;
            delete path[i];
            path[i] = NULL;
        }
        path[i] = FetchNode(r);
        // pin
        pfHandle->pin(path[i]->GetPageRID().page);
        pathP[i-1] = path[i-1]->GetNumKeys() - 1;
    }
    return path[hdr.height-1];
}

// 没找到返回NULL，找的同时会更新path

BtreeNode* IXIndexHandle::FindLeaf(const void *pData)
{
    RC invalid = IsValid(); if(invalid) return NULL;
    if (root == NULL) return NULL;
    RID addr;
    //只有一个根节点
    if(hdr.height == 1) {
        path[0] = root;
        return root;
    }
    //在open的时候path[0]会被设为root
    for (int i = 1; i < hdr.height; i++)
    {
        //key[pos-1]<key<=key[pos]
        RID r = path[i-1]->FindAddrAtPosition(pData);
        int pos = path[i-1]->FindKeyPosition(pData);
        //key比记录中所有key都大
        if(r.page == -1)
        {
            //取出索引中最大key
            const void * p = path[i-1]->LargestKey();
            //新的r和pos
            r = path[i-1]->FindAddr((const void*&)(p));
            pos = path[i-1]->FindKey((const void*&)(p));
        }
        //之前path[i]有值
        if(path[i] != NULL)
        {
            RC rc = pfHandle->unpinpage(path[i]->GetPageRID().page);
            delete path[i];
            path[i] = NULL;
        }
        path[i] = FetchNode(r.page);//把page对应的btree node放到path中
        //pin
        RC rc = pfHandle->pin(path[i]->GetPageRID().page);
        rc = pfHandle->markDirty(path[i]->GetPageRID().page);
        pathP[i-1] = pos;
    }


   //最后一个节点就是相应的node
    return path[hdr.height-1];
}

// 找到相应相应页号的btreenode
//btreenode的数据都在pfpage里，其他的一些值重新建立就行了
BtreeNode* IXIndexHandle::FetchNode(PageNum p) const
{
    return FetchNode(RID(p,-1));
}

//根据rid新建一个node
BtreeNode* IXIndexHandle::FetchNode(RID r) const
{
    RC invalid = IsValid();
    if(invalid)
        return NULL;
    if(r.page < 0)
        return NULL;
    PFPageHandle ph;
    RC rc = GetThisPage(r.page, ph);
    if(rc!=0)
        return NULL;
    return new BtreeNode(hdr.attrType, hdr.attrLength, ph, false, hdr.pageSize);
}

// 找key和rid
RC IXIndexHandle::Search(void *pData, RID &rid)
{
    RC invalid = IsValid(); if(invalid) return invalid;
    if(pData == NULL)
        return IX_BADKEY;
    BtreeNode * leaf = FindLeaf(pData);
    if(leaf == NULL)
        return IX_BADKEY;
    rid = leaf->FindAddr((const void*&)pData);
    if(rid == RID(-1, -1))
        return IX_KEYNOTFOUND;
    return 0;
}


int IXIndexHandle::GetHeight() const
{
    return hdr.height;
}
void IXIndexHandle::SetHeight(const int& h)
{
    for(int i = 1;i < hdr.height; i++)
        if (path != NULL && path[i] != NULL) {
            delete path[i];
            path[i] = NULL;
        }
    if(path != NULL) delete [] path;
    if(pathP != NULL) delete [] pathP;

    hdr.height = h;
    path = new BtreeNode* [hdr.height];
    for(int i=1;i < hdr.height; i++)
        path[i] = NULL;
    path[0] = root;
    pathP = new int [hdr.height-1]; // leaves don't point
    for(int i=0;i < hdr.height-1; i++)
        pathP[i] = -1;
}

BtreeNode* IXIndexHandle::GetRoot() const
{
    return root;
}



