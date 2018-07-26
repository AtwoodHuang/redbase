#ifndef REDBASE_IX_COMMON_H
#define REDBASE_IX_COMMON_H

#include "common_thing.h"

// PageNum: page在文件中的编号
typedef int PageNum;

// SlotNum: record在文件中的号
typedef int SlotNum;

struct IXFileHdr {
    int numPages;      // 文件中页的数目
    int pageSize;      // 每一页大小（4092）
    PageNum rootPage;  // 根页号
    int pairSize;      // 键值对大小(key, RID)
    int maxnum;         // b数容量
    int height;        // b树的高度
    AttrType attrType; //键的类型
    int attrLength;  //键的长度
};
#endif //REDBASE_IX_COMMON_H
