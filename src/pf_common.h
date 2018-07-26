#ifndef REDBASE_PF_COMMON_H
#define REDBASE_PF_COMMON_H

#define PF_PAGE_SIZE  4092
#define page_num 40


struct PF_File_Head
{
    int free; //free和下面一样
    unsigned int size; //页的数目
};

#define PF_PAGE_LIST_END	-1			// end of list of free_ pages
#define PF_PAGE_USED		-2			// Page is being used


struct PF_Page_Head
{
    int free;		// free可以有以下几种取值:
    // - 下一个空闲页的编号,此时该页面也是空闲的
    // - PF_PAGE_LIST_END => 页面是最后一个空闲页面
    // - PF_PAGE_USED => 页面并不是空闲的
};


const int PF_FILE_HDR_SIZE = PF_PAGE_SIZE + sizeof(PF_Page_Head);//为了保证对其，文件头大小实际上为一页
#endif //REDBASE_PF_COMMON_H
