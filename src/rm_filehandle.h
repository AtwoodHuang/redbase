#ifndef REDBASE_RM_FILEHANDLE_H
#define REDBASE_RM_FILEHANDLE_H

#include "common_thing.h"
#include "pf_filehandle.h"
#include "rm_rid.h"
#include "rm_commom.h"
#include "rm_record.h"
#include "rm_pagehdr.h"
#include "pf_pagehandle.h"

class RMFileHandle {
    friend class RMFileScan;
    friend class RMManager;
public:
    RMFileHandle():capacity_(0), pffile_(NULL), rcdlen_(0), changed_(false){}
    ~RMFileHandle();
public:
    RC open(PFFileHandle*);
    RC getRcd(const RID &rid, RMRecord &rcd);
    uint pagesSize() const
    {
        return rmhdr_.size;
    }
    RC insertRcd(const char* addr, RID &rid);
    RC deleteRcd(const RID &rid);
    RC updateRcd(const RMRecord &rcd);
    RC forcePages(PageNum num = -1);
    RC GetFileHeader(PFPageHandle ph);
    RC GetPageHeader(PFPageHandle ph, RMPageHdr& pHdr) const;
    RC SetPageHeader(PFPageHandle ph, const RMPageHdr& pHdr);
    RC SetFileHeader(PFPageHandle ph) const;
private:
    bool isValidPage(PageNum num) const;
    bool isValidRID(const RID& rid) const;

    bool nextFreeSlot(PFPageHandle& page, PageNum &num, SlotNum &slot);
    RC GetSlotPointer(PFPageHandle ph, SlotNum s, char *& pData) const;
public:
    uint capacity_;		// 每个页可支持的的slot的数目
    PFFileHandle *pffile_;
    RMFileHdr rmhdr_;   //rm文件头
    uint rcdlen_;		// 每一条记录的长度
    bool changed_;		// 文件头信息是否已经改变
};


#endif //REDBASE_RM_FILEHANDLE_H
