#ifndef REDBASE_PRINTER_H
#define REDBASE_PRINTER_H


#include "data_attr.h"
#include <vector>

#define MAXPRINTLEN ((2 * MAXNAME) + 5)



class Printer {
public:
    Printer(const DataAttrInfo *attrs, int nattrs);
    ~Printer() {};
public:
    void printHeader();
    void print(char* data);
    void printFooter();
private:

private:
    const DataAttrInfo *attrs_; //属性
    int count_; //属性数
    std::vector<int> colWidth_;	/* 用于记录没一行应当输出的空格的数目 */
    std::vector<char *> infos_;
    int printed_ = 0;			/* 已经输出的行数 */
};



#endif //REDBASE_PRINTER_H
