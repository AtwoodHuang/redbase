#ifndef REDBASE_RM_COMMOM_H
#define REDBASE_RM_COMMOM_H
struct RMFileHdr
{
    int free;				// 链表中第一个空闲页
    int size;				// 文件中已经分配了的页的数目
    uint rcdlen;			// 记录的长度
};

#define	RM_PAGE_LIST_END	-1
#define	RM_PAGE_FULLY_USED -2

#endif //REDBASE_RM_COMMOM_H
