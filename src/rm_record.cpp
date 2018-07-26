#include <cstring>
#include "rm_record.h"
#include "rm_error.h"




RMRecord::RMRecord() :recordsize(-1),data(NULL), rid(-1,-1)
{

}

RMRecord::~RMRecord()
{
    if(data != NULL)
    {
        delete[] data;
    }
}


//设定大小，id数据
RC RMRecord::set(char *pData, int size, RID id)
{
    if(recordsize != -1 && (size != recordsize))
        return RM_RECSIZEMISMATCH;
    recordsize = size;
    this->rid = id;
    if (data == NULL)
        data = new char[recordsize];
    memcpy(data, pData, size);
    return 0;
}


RC RMRecord::GetData(char *&pData) const
{
    if (data != NULL && recordsize != -1)
    {
        pData = data;
        return 0;
    }
    else
        return RM_NULLRECORD;
}

RC RMRecord::GetRid(RID &rid) const
{
    if (data != NULL && recordsize != -1)
    {
        rid = this->rid;
        return 0;
    }
    else
        return RM_NULLRECORD;

}

std::ostream& operator <<(std::ostream & os, const RID& r)
{
    PageNum p;
    SlotNum s;
    r.GetPageNum(p);
    r.GetSlotNum(s);
    os << "[" << p << "," << s << "]";
    return os;
}

