#include "ix_manager.h"
#include "ix_error.h"
#include "sstream"

IXManager::IXManager(PFManager & pfm):
        pfm(pfm)
{

}

IXManager::~IXManager()
{

}


//创建一个b+树索引,一个索引就是一个文件
RC IXManager::CreateIndex (const char *fileName, int indexNo, AttrType attrType, int attrLength, int pageSize)
{
    if(indexNo < 0 || attrType < INT || attrType > STRING || fileName == NULL)
        return IX_FCREATEFAIL;

    if(attrLength >= pageSize - (int)sizeof(RID) || attrLength <= 0)
        return IX_INVALIDSIZE;

    if((attrType == INT && (unsigned int)attrLength != sizeof(int)) ||
       (attrType == FLOAT && (unsigned int)attrLength != sizeof(float))
       ||
       (attrType == STRING &&
        ((unsigned int)attrLength <= 0 ||
         (unsigned int)attrLength > MAXSTRINGLEN)))
        return IX_FCREATEFAIL;

    std::stringstream newname;
    newname << fileName << "." << indexNo;
    //创建一个pffile，先新建一页放ixindex头
    RC rc = pfm.createFile(newname.str().c_str());
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }

    PFFileHandle pfh;
    rc = pfm.openFile(newname.str().c_str(), pfh);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }

    PFPageHandle headerPage;
    char * pData;

    rc = pfh.allocPage(headerPage);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    rc = headerPage.GetData(pData);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    IXFileHdr hdr;
    hdr.numPages = 1;
    hdr.pageSize = pageSize;
    hdr.pairSize = attrLength + sizeof(RID);
    hdr.maxnum = -1;
    hdr.height = 0;//初始高度为0
    hdr.rootPage = -1;
    hdr.attrType = attrType;
    hdr.attrLength = attrLength;

    memcpy(pData, &hdr, sizeof(hdr));
    PageNum headerPageNum;
    headerPage.GetPageNum(headerPageNum);

    rc = pfh.markDirty(headerPageNum);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    rc = pfh.unpinpage(headerPageNum);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    rc = pfm.closeFile(pfh);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    return (0);
}


RC IXManager::DestroyIndex (const char *fileName, int indexNo)
{
    if(indexNo < 0 ||
       fileName == NULL)
        return IX_FCREATEFAIL;

    std::stringstream newname;
    newname << fileName << "." << indexNo;

    RC rc = pfm.destroyFile(newname.str().c_str());
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    return 0;
}


RC IXManager::OpenIndex (const char *fileName, int indexNo, IXIndexHandle &ixh)
{
    if(indexNo < 0 ||
       fileName == NULL)
        return IX_FCREATEFAIL;

    PFFileHandle pfh;
    std::stringstream newname;
    newname << fileName << "." << indexNo;

    RC rc = pfm.openFile(newname.str().c_str(), pfh);
    if (rc < 0)
    {
        PFPrintError(rc);
        return IX_PF;
    }
    rc = ixh.Open(&pfh);
    if (rc < 0)
    {
        IXPrintError(rc);
        return rc;
    }

    return 0;
}

//
// CloseIndex

RC IXManager::CloseIndex(IXIndexHandle &ixh)
{
    if(!ixh.bFileOpen || ixh.pfHandle == NULL)
        return IX_FNOTOPEN;

    if(ixh.HdrChanged())
    {

        PFPageHandle ph;
        RC rc = ixh.pfHandle->getthisPage(0, ph);
        if (rc < 0)
        {
            PFPrintError(rc);
            return rc;
        }
        //先把头写进去
        rc = ixh.SetFileHeader(ph);
        if (rc < 0)
        {
            PFPrintError(rc);
            return rc;
        }

        rc = ixh.pfHandle->markDirty(0);
        if (rc < 0)
        {
            PFPrintError(rc);
            return rc;
        }

        rc = ixh.pfHandle->unpinpage(0);
        if (rc < 0)
        {
            PFPrintError(rc);
            return rc;
        }
        rc = ixh.ForcePages();
        if (rc < 0)
        {
            IXPrintError(rc);
            return rc;
        }
    }

    RC rc2 = pfm.closeFile(*ixh.pfHandle);
    if (rc2 < 0) {
        PFPrintError(rc2);
        return rc2;
    }
    ixh.~IXIndexHandle();
    ixh.pfHandle = NULL;
    ixh.bFileOpen = false;
    return 0;
}
