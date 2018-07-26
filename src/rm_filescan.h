#ifndef RM_FILESCAN_H
#define RM_FILESCAN_H
#include "rm_commom.h"
#include "rm_error.h"
#include "rm_filehandle.h"
#include "rm_record.h"
#include "comp.h"

//
// RMFileScan - 用作记录的扫描
//
class RMFileScan {
public:
    RMFileScan()
            : comp_(NULL), curr_(1, -1), op_(NO_OP)
            , offset_(0), val_(NULL) {};
    ~RMFileScan() {};
public:
    RC openScan(const RMFileHandle& rmfile, AttrType attr, int len, int offset, CompOp op,const void* val);
    RC getNextRcd(RMRecord &rcd);
    RC rewind();
    RC closeScan();
    RMFileHandle *rmfile_;
private:
    Comp *comp_;
    int offset_;
    CompOp op_;
    const void* val_;
    RID curr_;//文件的第0页是rm头，所以初始化时page为1

};

#endif /* RM_FILESCAN_H */