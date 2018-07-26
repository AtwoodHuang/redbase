#ifndef REDBASE_PF_BUFFER_H
#define REDBASE_PF_BUFFER_H

#include <memory.h>
#include "pf_common.h"
#include "common_thing.h"
#include "pf_hashtable.h"
#include "pf_error.h"

#define INVALID_SLOT  -1

//buffernode 即正好一个page
struct BufferNode {
    bool dirty = false;
    unsigned int count = 0;//引用数，当其为0且为脏就会被写入磁盘
    int fd = -1; //文件描述符
    int num = -1;//页号
    char buffer[ PF_PAGE_SIZE+ sizeof(PF_Page_Head)]; //一个page正好4k
};


class PFBuffer {

public:

    PFBuffer() : table(page_num/2) {}
    ~PFBuffer()
    {
        for(int i = 0;i< 40;++i)
        {
            if(nodes[i].fd != -1)
            {
                forcePages(nodes[i].fd, nodes[i].num);
            }
        }

    }

    RC getPage(int fd, int  num, char* &addr);
    RC allocPage(int fd, int num, char* &addr);
    RC markDirty(int fd, int num);
    RC unpin(int fd, int num);
    RC pin(int fd, int num);
    RC forcePages(int fd, int num);
    void clearFilePages(int fd);
    RC flush(int fd);
private:
    int searchAvaliableNode();
    RC readPage(int fd, int num, char* dst);
    RC writeBack(int fd, int num, char* src);

    BufferNode nodes[page_num];//每一个缓存有40个node,即缓冲区中有40个page,但是这40个page不来自于同一个文件，所以需要hashtable
    PFHashTable table;
};
#endif //REDBASE_PF_BUFFER_H
