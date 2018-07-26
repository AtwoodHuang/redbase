#ifndef REDBASE_PF_FILEHANDLE_H
#define REDBASE_PF_FILEHANDLE_H
#include "pf_buffer.h"
#include "pf_pagehandle.h"
#include <memory>
#include <iostream>
#include "pf_error.h"

//filehandle真正的信息只有一个文件头，它通过buff管理page
class PFFileHandle{
    friend class PFManager;
public:
    PFFileHandle() :opened(false), buff(NULL) {}
    PFFileHandle(const PFFileHandle& fileHandle);
    PFFileHandle& operator= (const PFFileHandle& fileHandle);
    ~PFFileHandle()
    {
       // flush();
    };
public:
    RC getfirstPage(PFPageHandle &page) const;
    RC getnextPage(int curr, PFPageHandle &page) const;
    RC getthisPage(int num, PFPageHandle &page) const;
    RC getlastPage(PFPageHandle &page) const;
    RC getprevPage(int curr, PFPageHandle &page) const;

    RC allocPage(PFPageHandle &page);
    RC disposePage(int num);
    RC markDirty(int num) const;
    RC unpinpage(int num) const;
    RC pin(int num);
    RC forcePages(int num = -1);
    PFBuffer *buff;				// PFBuffer这个类用于管理缓存
private:
    void clearFilePages();
    RC flush();
private:

    PF_File_Head hdr;					// 用于记录文件头部的信息
    bool opened;					// 文件是否已经打开
    bool changed;					// 文件头部是否已经改变了
    int fd;						// 文件描述符
};



#endif //REDBASE_PF_FILEHANDLE_H
