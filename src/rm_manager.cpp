

#include "rm_manager.h"


RC RMManager::createFile(const char* pathname, unsigned int recordsize)
{
    if (recordsize == 0)
        return RM_BADRECSIZE;
    if (recordsize >= (PF_PAGE_SIZE - sizeof(RMPageHdr)))
        return RM_SIZETOOBIG;
    RC rc = pfm.createFile(pathname);
    if(rc <0 )
    {
        PFPrintError(rc);
        return  rc;
    }

    PFFileHandle file;
    rc = pfm.openFile(pathname, file);
    if(rc <0 )
    {
        PFPrintError(rc);
        return  RM_PF;
    }

    PFPageHandle headerpage;
    char* pData;
    rc = file.allocPage(headerpage);
    if(rc <0 )
    {
        PFPrintError(rc);
        return  RM_PF;
    }

    rc = headerpage.GetData(pData);
    if(rc < 0)
    {
        PFPrintError(rc);
        return  RM_PF;
    }

    RMFileHdr hdr;      //第0页仅仅用作RM文件头
    hdr.free = RM_PAGE_LIST_END;
    hdr.size = 1; //已份配的页数
    hdr.rcdlen = recordsize; // 记录大小

    memcpy(pData, &hdr, sizeof(hdr));

    int pageNum;
    headerpage.GetPageNum(pageNum);

    file.markDirty(pageNum);
    file.unpinpage(pageNum);

    pfm.closeFile(file);
    return 0;


}

RC RMManager::destroyFile(const char* pathname)
{
    RC rc = pfm.destroyFile((pathname));
    if(rc < 0)
    {
        PFPrintError(rc);
        return  RM_PF;
    }
    return 0;
}

//
// openFile - 打开某个文件
//
RC RMManager::openFile(const char* pathname, RMFileHandle &rmFile)
{
    PFFileHandle pfh;
    RC rc = pfm.openFile(pathname, pfh);
    if(rc < 0)
    {
        PFPrintError(rc);
        return  RM_PF;
    }
    rc = rmFile.open(&pfh);
    if (rc < 0)
    {
        RMPrintError(rc);
        return rc;
    }
    return 0;

}

//
// closeFile 将文件关闭
//
RC RMManager::closeFile(RMFileHandle& file)
{
    if(file.pffile_ == NULL)
        return RM_FNOTOPEN;

    if(file.changed_)
    {
        // 文件已改变，写入硬盘
        PFPageHandle ph;
        file.pffile_->getthisPage(0, ph);
        char * buf;
        RC rc = ph.GetData(buf);
        memcpy(buf, &file.rmhdr_, sizeof(RMFileHdr));

        rc = file.pffile_->markDirty(0);
        if (rc < 0)
        {
            PFPrintError(rc);
            return rc;
        }

        rc = file.pffile_->unpinpage(0);
        if (rc < 0)
        {
            PFPrintError(rc);
            return rc;
        }

        rc = file.forcePages();
        if (rc < 0)
        {
            RMPrintError(rc);
            return rc;
        }
    }

    RC rc2 = pfm.closeFile(*file.pffile_);
    if (rc2 < 0) {
        PFPrintError(rc2);
        return rc2;
    }
    delete file.pffile_;
    file.pffile_ = NULL;
    return 0;
}