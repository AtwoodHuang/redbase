#include "pf_pagehandle.h"
#include "pf_filehandle.h"

PFPageHandle::~PFPageHandle()
{
}

//
// operator= - 赋值运算符
//
PFPageHandle& PFPageHandle::operator=(const PFPageHandle& rhs)
{
    if (this != &rhs)
    {
        this->num = rhs.num;
        this->addr = rhs.addr;
    }
    return *this;
}

//
// PFPageHandle - 拷贝构造函数
//
PFPageHandle::PFPageHandle(const PFPageHandle& rhs)
{
    this->num = rhs.num;
    this->addr = rhs.addr;
}

RC PFPageHandle::GetData(char *&pData) const
{
    if (addr == NULL)
        return PF_PAGEUNPINNED;
    pData = addr;

    return (0);
}

RC PFPageHandle::GetPageNum(int &pageNum) const
{
    if(addr == NULL)
        return PF_PAGEUNPINNED;
    pageNum = num;

    return 0;
}