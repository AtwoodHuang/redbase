#ifndef REDBASE_DATA_ATTR_H
#define REDBASE_DATA_ATTR_H

#include <cstring>
#include "common_thing.h"

//属性信息
struct DataAttrInfo
{
    // 默认构造函数
    DataAttrInfo() {
        memset(relName, 0, MAXNAME + 1);
        memset(attrName, 0, MAXNAME + 1);
        offset = -1;
    };
    // Copy constructor
    DataAttrInfo( const DataAttrInfo &d ) {
        strcpy (relName, d.relName);
        strcpy (attrName, d.attrName);
        offset = d.offset;
        attrType = d.attrType;
        attrLength = d.attrLength;
        indexNo = d.indexNo;
    };

    DataAttrInfo& operator=(const DataAttrInfo &d) {
        if (this != &d) {
            strcpy (relName, d.relName);
            strcpy (attrName, d.attrName);
            offset = d.offset;
            attrType = d.attrType;
            attrLength = d.attrLength;
            indexNo = d.indexNo;
        }
        return (*this);
    };

    int      offset;                // 在表中的偏移
    AttrType attrType;              // 属性
    int      attrLength;            // 属性长度
    int      indexNo;               // 属性编号
    char     relName[MAXNAME+1];    // 表的名字
    char     attrName[MAXNAME+1];   // 属性的名字
};

//表的信息
struct DataRelInfo
{
    // Default constructor
    DataRelInfo() {
        memset(relName, 0, MAXNAME + 1);
    }

    // Copy constructor
    DataRelInfo( const DataRelInfo &d ) {
        strcpy (relName, d.relName);
        recordSize = d.recordSize;
        attrCount = d.attrCount;
        numPages = d.numPages;
        numRecords = d.numRecords;
    };

    DataRelInfo& operator=(const DataRelInfo &d) {
        if (this != &d) {
            strcpy (relName, d.relName);
            recordSize = d.recordSize;
            attrCount = d.attrCount;
            numPages = d.numPages;
            numRecords = d.numRecords;
        }
        return (*this);
    }

    int      recordSize;            // 记录的长度
    int      attrCount;             // 属性的数量
    int      numPages;              // 使用的页的数量
    int      numRecords;            //
    char     relName[MAXNAME+1];    // 表名
};




#endif
