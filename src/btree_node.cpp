#include "btree_node.h"
#include "cassert"
#include "cmath"
#include "ix_error.h"

// 如果是叶子节点，RID(i)指向keys(i)对应的值
// 不是叶子节点，RID(i) 指向key(i-1)<keys <= keys(i)的节点
BtreeNode::BtreeNode(AttrType attrType, int attrLength,
                     PFPageHandle& ph, bool newPage,
                     int pageSize)
        :keys(NULL), rids(NULL),
         attrLength(attrLength), attrType(attrType)
{
    //计算一个pfpage能放多少个node,先是order个key，后是order个rid，紧跟着是key的个数，最后是left和right
    maxnum = floor((pageSize - sizeof(numKeys) - 2*sizeof(PageNum)) / (sizeof(RID) + attrLength));
    // 调整一下，确保不超界
    while( ((maxnum) * (attrLength + sizeof(RID)))
           > ((unsigned int) pageSize - sizeof(numKeys) - 2*sizeof(PageNum) ))
        maxnum--;


    char * pData = NULL;
    ph.GetData(pData);//取出PFpage里的数据
    PageNum p;
    ph.GetPageNum(p);
    SetPageRID(RID(p, -1));//设定该叶子所在page

    keys = pData;
    rids = (RID*) (pData + attrLength*(maxnum));//先是n个key，后面再跟n个rid
    // 该page已经初始化过了
    if(!newPage)
    {
        GetNumKeys();//有多少个key
        GetLeft();
        GetRight();
    }
    else {//还没初始化
        // 自己设值
        SetNumKeys(0);
        SetLeft(-1);
        SetRight(-1);
    }
}

BtreeNode::~BtreeNode()
{
};

//btree node 储存在pfpage里
RC BtreeNode::ResetBtreeNode(PFPageHandle& ph, const BtreeNode& rhs)
{
    maxnum = (rhs.maxnum);
    attrLength = (rhs.attrLength);
    attrType = (rhs.attrType);
    numKeys = (rhs.numKeys);

    char * pData = NULL;
    RC rc = ph.GetData(pData);
    if(rc != 0 )
        return rc;

    PageNum p;
    rc = ph.GetPageNum(p);
    if(rc != 0 )
        return rc;
    SetPageRID(RID(p, -1));

    keys = pData;//开始就是n个key
    rids = (RID*) (pData + attrLength*(maxnum));//紧跟着n个rid

    GetNumKeys();
    GetLeft();
    GetRight();

    assert(IsValid() == 0);
    return 0;
};

int BtreeNode::Destroy()
{
    assert(IsValid() == 0);
    if(numKeys != 0)
        return -1;

    keys = NULL;
    rids = NULL;
    return 0;
}

int BtreeNode::GetNumKeys()
{
    assert(IsValid() == 0);
    void * loc = (char*)rids + sizeof(RID)*maxnum;//先是key，后是rid，紧跟着是key的个数
    int * pi = (int *) loc;
    numKeys = *pi;
    return numKeys;
};


int BtreeNode::SetNumKeys(int newNumKeys)
{
    memcpy((char*)rids + sizeof(RID)*maxnum, &newNumKeys, sizeof(int));
    numKeys = newNumKeys;
    assert(IsValid() == 0);
    return 0;
}

PageNum BtreeNode::GetLeft()
{
    assert(IsValid() == 0);
    void * loc = (char*)rids + sizeof(RID)*maxnum + sizeof(int);
    return *((PageNum*) loc);
};

int BtreeNode::SetLeft(PageNum p)
{
    assert(IsValid() == 0);
    memcpy((char*)rids + sizeof(RID)*maxnum + sizeof(int),
           &p,
           sizeof(PageNum));
    return 0;
}

PageNum BtreeNode::GetRight()
{
    assert(IsValid() == 0);
    void * loc = (char*)rids + sizeof(RID)*maxnum + sizeof(int) + sizeof(PageNum);
    return *((PageNum*) loc);
};

int BtreeNode::SetRight(PageNum p)
{
    assert(IsValid() == 0);
    memcpy((char*)rids + sizeof(RID)*maxnum + sizeof(int)  + sizeof(PageNum),
           &p,
           sizeof(PageNum));
    return 0;
}


RID BtreeNode::GetPageRID() const
{
    return pageRID;
}
void BtreeNode::SetPageRID(const RID& r)
{
    pageRID = r;
}



RC BtreeNode::IsValid() const
{
    if (maxnum <= 0)
        return IX_INVALIDSIZE;

    bool ret = true;
    ret = ret && (keys != NULL);
    ret = ret && (rids != NULL);
    ret = ret && (numKeys >= 0);
    ret = ret && (numKeys <= maxnum);
    if(!ret)
        std::cerr << "order was " << maxnum << " numkeys was  " << numKeys << std::endl;
    return ret ? 0 : IX_BADIXPAGE;
};


int BtreeNode::GetMaxKeys() const
{
    assert(IsValid() == 0);
    return maxnum;
};


// 最大的key在末尾
void* BtreeNode::LargestKey() const
{
    assert(IsValid() == 0);
    void * key = NULL;
    if (numKeys > 0) {
        GetKey(numKeys-1, key);
        return key;
    } else {
        return NULL;
    }
};



// 找到第pos个key
RC BtreeNode::GetKey(int pos, void* &key) const
{
    assert(IsValid() == 0);
    assert(pos >= 0 && pos < numKeys);
    if (pos >= 0 && pos < numKeys)
    {
        key = keys + attrLength*pos;
        return 0;
    }
    else
    {
        return -1;
    }
}

// 把pos上的key放到tokey上
int BtreeNode::CopyKey(int pos, void* toKey) const
{
    assert(IsValid() == 0);
    assert(pos >= 0 && pos < maxnum);
    if(toKey == NULL)
        return -1;
    if (pos >= 0 && pos < maxnum)
    {
        memcpy(toKey, keys + attrLength*pos, attrLength);
        return 0;
    }
    else
    {
        return -1;
    }
}

//newkey放到pos上
int BtreeNode::SetKey(int pos, const void* newkey)
{
    assert(IsValid() == 0);
    assert(pos >= 0 && pos < maxnum);
    if(newkey == (keys + attrLength*pos))
        return 0;

    if (pos >= 0 && pos < maxnum)
    {
        memcpy(keys + attrLength*pos, newkey, attrLength);
        return 0;
    }
    else
    {
        return -1;
    }
}

//插入key和相应的rid,0成功，-1失败
int BtreeNode::Insert(const void* newkey, const RID & rid)
{
    assert(IsValid() == 0);
    if(numKeys >= maxnum)
        return -1;
    int i = -1;
    void *currKey = NULL;
    for(i = numKeys-1; i >= 0; i--)
    {
        GetKey(i, currKey);
        if (CmpKey(newkey, currKey) >= 0)
            break;
        //在比较的同时将rid和key向右移
        rids[i+1] = rids[i];
        SetKey(i + 1, currKey);
    }
    rids[i+1] = rid;
    SetKey(i+1, newkey);

    assert(isSorted());
    //增加numkey
    SetNumKeys(GetNumKeys()+1);
    return 0;
}

// 返回0,成功
// 返回-2,key不存在
// 返回-1,删完后为空
//如果给了kpos就会删掉kpos，没给就会查找相应的newkey
int BtreeNode::Remove(const void* newkey, int kpos)
{
    assert(IsValid() == 0);
    int pos = -1;
    if (kpos != -1) {
        if (kpos < 0 || kpos >= numKeys)
            return -2;
        pos = kpos;
    }
    else {
        pos = FindKey(newkey);
        if (pos < 0)
            return -2;
    }
    for(int i = pos; i < numKeys-1; i++)
    {
        //删除时向左移
        void *p;
        GetKey(i+1, p);
        SetKey(i, p);
        rids[i] = rids[i+1];
    }
    SetNumKeys(GetNumKeys()-1);
    if(numKeys == 0) return -1;
    return 0;
}

//找key对应的rids
RID BtreeNode::FindAddrAtPosition(const void* &key) const
{
    assert(IsValid() == 0);
    int pos = FindKeyPosition(key);
    if (pos == -1 || pos >= numKeys)
        return RID(-1,-1);
    return rids[pos];
}

//找key所在的pos，不一定等于key,但肯定大于key
int BtreeNode::FindKeyPosition(const void* &key) const
{
    assert(IsValid() == 0);
    for(int i = numKeys-1; i >=0; i--)
    {
        void* k;
        if(GetKey(i, k) != 0)
            return -1;
        if (CmpKey(key, k) == 0)
            return i;
        if (CmpKey(key, k) > 0)
            return i+1;
    }
    return 0;
}

// 根据pos，返回rid
RID BtreeNode::GetAddr(const int pos) const
{
    assert(IsValid() == 0);
    if(pos < 0 || pos > numKeys)
        return RID(-1, -1);
    return rids[pos];
}

// 根据key返回rid
RID BtreeNode::FindAddr(const void* &key) const
{
    assert(IsValid() == 0);
    int pos = FindKey(key);
    if (pos == -1) return RID(-1,-1);
    return rids[pos];
}


//给key和rid 返回位置
int BtreeNode::FindKey(const void* &key, const RID& r) const
{
    assert(IsValid() == 0);

    for(int i = numKeys-1; i >= 0; i--)
    {
        void* k;
        if(GetKey(i, k) != 0)
            return -1;
        if (CmpKey(key, k) == 0) {
            if(r == RID(-1,-1))
                return i;
            else {
                if (rids[i] == r)
                    return i;
            }
        }
    }
    return -1;
}

//比较函数
int BtreeNode::CmpKey(const void * a, const void * b) const
{
    if (attrType == STRING) {
        return memcmp(a, b, attrLength);
    }

    if (attrType == FLOAT) {
        typedef float MyType;
        if ( *(MyType*)a >  *(MyType*)b ) return 1;
        if ( *(MyType*)a == *(MyType*)b ) return 0;
        if ( *(MyType*)a <  *(MyType*)b ) return -1;
    }

    if (attrType == INT) {
        typedef int MyType;
        if ( *(MyType*)a >  *(MyType*)b ) return 1;
        if ( *(MyType*)a == *(MyType*)b ) return 0;
        if ( *(MyType*)a <  *(MyType*)b ) return -1;
    }
    return 0;
}

bool BtreeNode::isSorted() const
{
    assert(IsValid() == 0);

    for(int i = 0; i < numKeys-2; i++)
    {
        void* k1;
        GetKey(i, k1);
        void* k2;
        GetKey(i+1, k2);
        if (CmpKey(k1, k2) > 0)
            return false;
    }
    return true;
}


// b+数分裂
RC BtreeNode::Split(BtreeNode* rhs)
{
    int firstMovedPos = (numKeys+1)/2;//从中间分裂
    int moveCount = (numKeys - firstMovedPos);

    if( (rhs->GetNumKeys() + moveCount) > rhs->GetMaxKeys())
        return -1;

    for (int pos = firstMovedPos; pos < numKeys; pos++) {
        RID r = rids[pos];
        //找到pos个key和rid，插到rhs里
        void * k = NULL;
        this->GetKey(pos, k);//浅层拷贝
        RC rc = rhs->Insert(k, r);//深层拷贝
        if(rc != 0)
            return rc;
    }
    //把插入的元素从原位置删掉
    for (int i = 0; i < moveCount; i++) {
        RC rc = this->Remove(NULL, firstMovedPos);
        if(rc < -1) {
            return rc;
        }
    }
    //他们都是叶子节点，把他们连起来
    //请注意this右边的节点的左叶子应该设为rhs，但是该节点可能不存在，因此这个处理放在外面
    rhs->SetRight(this->GetRight());

    this->SetRight(rhs->GetPageRID().page);
    rhs->SetLeft(this->GetPageRID().page);



    assert(isSorted());
    assert(rhs->isSorted());

    assert(IsValid() == 0);
    assert(rhs->IsValid() == 0);
    return 0;
}

// 合并，other和该叶子必须相邻
RC BtreeNode::Merge(BtreeNode* other) {
    assert(IsValid() == 0);
    assert(other->IsValid() == 0);

    if (numKeys + other->GetNumKeys() > maxnum)
        return -1;

    for (int pos = 0; pos < other->GetNumKeys(); pos++) {
        void * k = NULL; other->GetKey(pos, k);
        RID r = other->GetAddr(pos);
        RC rc = this->Insert(k, r);
        if(rc != 0) return rc;
    }

    int moveCount = other->GetNumKeys();
    for (int i = 0; i < moveCount; i++) {
        RC rc = other->Remove(NULL, 0);
        if(rc < -1) return rc;
    }

    if(this->GetPageRID().page == other->GetLeft())
        this->SetRight(other->GetRight());
    else
        this->SetLeft(other->GetLeft());

    assert(IsValid() == 0);
    assert(other->IsValid() == 0);
    return 0;
}

void BtreeNode::Print(std::ostream & os) {
    os << GetLeft() << "<--"
       << pageRID.page << "{";
    for (int pos = 0; pos < GetNumKeys(); pos++) {
        void * k = NULL; GetKey(pos, k);
        os << "(";
        if( attrType == INT )
            os << *((int*)k);
        if( attrType == FLOAT )
            os << *((float*)k);
        if( attrType == STRING ) {
            for(int i=0; i < attrLength; i++)
                os << ((char*)k)[i];
        }
        os << ","
           << GetAddr(pos) << "), ";
    }
    if(numKeys > 0)
        os << "\b\b";
    os << "}"
       << "-->" << GetRight() << "\n";
}

