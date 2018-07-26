#include "pf_manager.h"
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>
#include <zconf.h>


RC PFManager::createFile(const char* pathname)
{
    int fd, n;
    fd = open(pathname, O_CREAT | O_EXCL | O_RDWR, S_IRUSR|S_IWUSR|S_IXUSR);
    char hdrBuf[PF_FILE_HDR_SIZE];
    memset(hdrBuf, 0, PF_FILE_HDR_SIZE);

    PF_File_Head *hd = (PF_File_Head*)hdrBuf;
    //这里create文件时文件头已经初始化过了，之后只需要把文件头读进来就行了
    hd->free = PF_PAGE_LIST_END;
    hd->size = 0;

    // 将头部写入到文件中
    n = write(fd, hdrBuf, PF_FILE_HDR_SIZE);
    if (n != PF_FILE_HDR_SIZE)
    {
        close(fd);
        unlink(pathname); // 删除文件
        if (n < 0)
            return PF_UNIX;
        else
            return PF_HDRWRITE;
    }
    close(fd);
    return 0;
}

RC PFManager::destroyFile(const char *pathname)
{
    unlink(pathname);
    return 0;
}

RC PFManager::openFile(const char* pathname, PFFileHandle &file)
{
    RC rc;
    file.fd = open(pathname, O_RDWR);
    //读进文件头
    int n = read(file.fd, (char*)&file.hdr, sizeof(PF_File_Head));

    if (n != sizeof(PF_File_Head))
    {
        close(file.fd);
        return PF_HDRREAD;
    }
    file.changed = false;
    file.opened = true;
    file.buff = buff;
    return 0;
}

RC PFManager::closeFile(PFFileHandle &file)
{
    //将buff刷新到磁盘上
    int fd = file.fd;
    file.flush();
    //从表中移除
    file.clearFilePages();
    close(fd);
    return 0;
}




