#ifndef REDBASE_RM_PAGEHDR_H
#define REDBASE_RM_PAGEHDR_H

#include <iostream>
#include "bitmap.h"
#include <cstring>

struct RMPageHdr {
    int free;       // 和common_thing里定义一样

    char * freeSlotMap; // 记录freeslot 的bitmap
    int numSlots;       //slot总数
    int numFreeSlots;  //空闲slot数

    RMPageHdr(int numSlots) : numSlots(numSlots), numFreeSlots(numSlots)
    {
        freeSlotMap = new char[this->mapsize()];
    }

    ~RMPageHdr()
    {
        delete [] freeSlotMap;
    }

    //总大小
    int size() const
    {
        return sizeof(free) + sizeof(numSlots) + sizeof(numFreeSlots) + bitmap(numSlots).numChars()*sizeof(char);
    }

    //bitmap大小
    int mapsize() const
    {
        return this->size() - sizeof(free) - sizeof(numSlots) - sizeof(numFreeSlots);
    }

    //内容放到buff里
    int to_buf(char *& buf) const
    {
        memcpy(buf, &free, sizeof(free));
        memcpy(buf + sizeof(free), &numSlots, sizeof(numSlots));
        memcpy(buf + sizeof(free) + sizeof(numSlots), &numFreeSlots, sizeof(numFreeSlots));
        memcpy(buf + sizeof(free) + sizeof(numSlots) + sizeof(numFreeSlots), freeSlotMap, this->mapsize()*sizeof(char));
        return 0;
    }

    //从buff读入
    int from_buf(const char * buf)
    {
        memcpy(&free, buf, sizeof(free));
        memcpy(&numSlots, buf + sizeof(free), sizeof(numSlots));
        memcpy(&numFreeSlots, buf + sizeof(free) + sizeof(numSlots), sizeof(numFreeSlots));
        memcpy(freeSlotMap, buf + sizeof(free) + sizeof(numSlots) + sizeof(numFreeSlots), this->mapsize()*sizeof(char));
        return 0;
    }
};
#endif //REDBASE_RM_PAGEHDR_H
