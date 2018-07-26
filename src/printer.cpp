#include "printer.h"

#include <iostream>
#include <string.h>
#include <ostream>
#include <algorithm>
#include <sstream>
#include "printer.h"
using namespace std;

static char infos[MAXATTRS][MAXPRINTLEN];


Printer::Printer(const DataAttrInfo *attrs, int nattrs)
        : count_(nattrs), printed_(0), attrs_(attrs)
{
    for (int i = 0; i < nattrs; i++) {
        sprintf(infos[i], "%s", attrs[i].attrName);
        int width;
        int len = strlen(infos[i]) + 1;
        if (attrs[i].attrType == STRING) {
            //因为要对齐，所以选属性名长度和内容长度中较大的
            width = max(len, attrs[i].attrLength);
            width = min(width, MAXPRINTLEN);
        }
        else {
            width = len > 12 ? len: 12;
        }
        infos_.push_back(infos[i]);
        colWidth_.push_back(width);
    }
}



//
// printHeader - 输出头部的信息
//
void Printer::printHeader()
{
    int len;
    int dashes = 0;
    int spaces = 0;
    for (int i = 0; i < infos_.size(); i++) {
        cout << infos_[i];
        len = strlen(infos_[i]);
        dashes += len;
        spaces = colWidth_[i] - len;
        //补齐空格
        for (int j = 0; j < spaces; j++) cout << " ";
        dashes += spaces;
    }
    cout << endl;
    //最后空格加上非空格个－
    for (int k = 0; k < dashes; k++) cout << "-";
    cout << endl;
}


void Printer::printFooter()
{
    cout << endl;
    cout <<printed_ << " tuple(s)." << endl;
}




void Printer::print(char* data)
{
    char buffer[MAXPRINTLEN];
    printed_++;
    for (int i = 0; i < count_; i++) {
        int space = 0;
        char *ptr = data + attrs_[i].offset;
        switch (attrs_[i].attrType)
        {
            case STRING:
                if (attrs_[i].attrLength > MAXPRINTLEN) {
                    memcpy(buffer, ptr, MAXPRINTLEN - 1);
                    buffer[MAXPRINTLEN - 1] = '\0';
                    buffer[MAXPRINTLEN - 3] = buffer[MAXPRINTLEN - 2] = '.';
                }
                else {
                    memcpy(buffer, ptr, attrs_[i].attrLength);
                }
                break;
            case INT:
            {
                int val = *reinterpret_cast<int *>(ptr);
                sprintf(buffer, "%d", val);
                break;
            }
            case FLOAT:
            {
                float fval = *reinterpret_cast<float *>(ptr);
                sprintf(buffer, "%f", fval);
                break;
            }
            default:
                break;
        }
        cout << buffer;
        space = colWidth_[i] - strlen(buffer);
        for (int j = 0; j < space; j++) cout << " ";
    }
    cout << endl;
}



