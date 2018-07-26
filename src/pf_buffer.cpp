#include <zconf.h>
#include <iostream>
#include "pf_buffer.h"
static int pagesize = PF_PAGE_SIZE + sizeof(PF_Page_Head);
static int filesize = PF_PAGE_SIZE + sizeof(PF_File_Head);


//
//  获取一个指向缓存空间的指针，如果页面已经在缓存之中的话，(re)pin 页面，然后返回一个
//	指向它的指针。如果页面没有在缓存中，将其从文件中读出来，pin it，然后返回一个指向它的指针。如果
//  缓存是满的话，替换掉一个页。
//
RC PFBuffer::getPage(int fd, int num, char* &addr)
{
    int idx = table.search(fd, num);
    if (idx >= 0)
    {
        // 页面已经在内存中了，现在只需要增加引用即可
        nodes[idx].count++;
    }
    else
    {
        // 分配一个空的页面,页面的在buff中的位置由idx记录
        idx = searchAvaliableNode();
        if (idx < 0)
            return PF_NOBUF;
        readPage(fd, num, nodes[idx].buffer);
        nodes[idx].fd = fd;
        nodes[idx].num = num;
        nodes[idx].count = 1;
        nodes[idx].dirty = false;
        table.insert(fd, num, idx);
    }
    // dst指针直接指向页的首部
    addr = nodes[idx].buffer;
    return 0;
}

//
// 将页面锁在缓冲区里,大致流程和getPage类似,就是增加引用计数
//
RC PFBuffer::pin(int fd, int num)
{
    int idx = table.search(fd, num);
    if (idx >= 0)
    {
        // 页面已经在内存中了，现在只需要增加引用即可
        nodes[idx].count++;
    }
    else
    {
        // 分配一个空的页面,页面的在buff中的位置由idx记录
        idx = searchAvaliableNode();
        if (idx < 0)
            return PF_NOBUF;
        readPage(fd, num, nodes[idx].buffer);
        nodes[idx].fd = fd;
        nodes[idx].num = num;
        nodes[idx].count = 1;
        nodes[idx].dirty = false;
        table.insert(fd, num, idx);
    }
    return 0;
}

//
// 分配一个页面
//该分配页面实际上分配的是buff中的内存
RC PFBuffer::allocPage(int fd, int num, char* &addr)
{
    int idx = table.search(fd, num); //在buff中该页不存在
    if (idx > 0)
        return PF_PAGEINBUF;

    // 分配空的页
    idx = searchAvaliableNode();
    if (idx < 0)
        return PF_NOBUF;

    table.insert(fd, num, idx);
    nodes[idx].fd = fd;
    nodes[idx].num = num;
    nodes[idx].dirty = false;
    nodes[idx].count = 1;
    addr = nodes[idx].buffer;
    return 0;
}

//
// 将一个页面标记为脏,当它从bufffer中丢弃的时候,它会被回到文件中
//
RC PFBuffer::markDirty(int fd, int num)
{
    int idx = table.search(fd, num);
    // 页面必须存在于缓存中
    if (idx < 0)
        return PF_PAGENOTINBUF;

    //if (nodes[idx].count == 0)
        //return PF_PAGEUNPINNED;
    // 标记为脏页
    nodes[idx].dirty = true;
    return 0;
}

//
// 减少一个引用量
//
RC PFBuffer::unpin(int fd, int num)
{
    int idx = table.search(fd, num);
    if (idx < 0)
        return PF_PAGENOTINBUF;
    nodes[idx].count -= 1;
    if (nodes[idx].count == 0) //引用计数等于0且为脏，会被写入磁盘
    {
        if (nodes[idx].dirty)
        {
            writeBack(nodes[idx].fd, nodes[idx].num, nodes[idx].buffer);
            nodes[idx].dirty = false;
        }
    }
    return 0;
}

//
// 将脏的页面全部都刷到磁盘中,且从hash表中移除
//
RC PFBuffer::flush(int fd)
{
    for (int i = 0; i < page_num; i++) {
        if (nodes[i].fd == fd)
        {
            if (nodes[i].dirty)
            {
                writeBack(fd, nodes[i].num, nodes[i].buffer);
                nodes[i].dirty = false;
            }
        }
    }
    clearFilePages(fd);
    return 0;
}

//
// 强制将某页面写入磁盘中
//
RC PFBuffer::forcePages(int fd, int num)
{
    for (int i = 0; i < page_num; i++)
    {
        if ((nodes[i].fd == fd) && (nodes[i].num == num || num == -1))//num等于-1,所有page都会被写入
        {
            if (nodes[i].dirty)
            {
                writeBack(fd, nodes[i].num, nodes[i].buffer);
                nodes[i].dirty = false;
            }
        }
    }
    return 0;
}

//
// 将所有缓存的页面全部移除
//
void PFBuffer::clearFilePages(int fd)
{
    for (int i = 0; i < page_num; i++)
    {
        if (nodes[i].fd == fd)
        {
            table.remove(nodes[i].fd, nodes[i].num);
            if (nodes[i].dirty)
            {
                writeBack(fd, nodes[i].num, nodes[i].buffer);
                nodes[i].dirty = false;
            }
            nodes[i].fd = -1;
            nodes[i].num = -1;
            nodes[i].count = 0;
        }
    }
}

//
// 先找没用到的，都用了就换页此处用到LRU，越是之前用到的页i越小
//
int PFBuffer::searchAvaliableNode()
{
    for (int i = 0; i < page_num; i++)
    {
        if (nodes[i].fd == -1)
            return  i;
    }
    for (int i = 0; i < page_num; i++)
    {
        if (nodes[i].count == 0)
        {
            table.remove(nodes[i].fd, nodes[i].num);
            if (nodes[i].dirty)
            {
                writeBack(nodes[i].fd, nodes[i].num, nodes[i].buffer);
                nodes[i].dirty = false;
            }
            nodes[i].fd = -1;
            nodes[i].num = -1;
            nodes[i].count = 0;
            return  i;
        }
    }
    return -1;
}

//
// 从磁盘中读取一个页面
//
RC PFBuffer::readPage(int fd, int num, char* dst)
{
    long offset = num * (long)(pagesize) + PF_FILE_HDR_SIZE; //num从0开始，最开始一个是file头，实际上一个file头大小为pagesize
    lseek(fd, offset, SEEK_SET);
    // 读取数据
    int n = read(fd, dst, pagesize);
    if (n != pagesize)
        return PF_INCOMPLETEREAD;
    return 0; // 一切正常
}


//
// 将src处的内容写回到磁盘
//
RC PFBuffer::writeBack(int fd, int num, char* src)
{
    // 这里的页面计数,从文件头部之后开始
    long offset = num * pagesize + PF_FILE_HDR_SIZE;
    lseek(fd, offset, SEEK_SET);

    int n = write(fd, src, pagesize);
    if (n != pagesize)
        return PF_INCOMPLETEWRITE;
    return 0;
}