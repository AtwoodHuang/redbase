#ifndef REDBASE_PF_PAGEHANDLE_H
#define REDBASE_PF_PAGEHANDLE_H

#include <memory>
#include "common_thing.h"
#include "pf_error.h"

class PFFileHandle;

class PFPageHandle {
    friend class PFFileHandle;
public:
    PFPageHandle() : num(-1), addr(NULL) {}
    ~PFPageHandle();
    PFPageHandle(const PFPageHandle& rhs);
    PFPageHandle& operator=(const PFPageHandle &page);

    RC GetData     (char *&pData) const;           // Set pData to point to
    RC GetPageNum  (int &pageNum) const;       // Return the page number
private:
    int  num;						// 页面的编号
    char* addr;						// 指向实际的页面数据,不包含头
};



#endif //REDBASE_PF_PAGEHANDLE_H
