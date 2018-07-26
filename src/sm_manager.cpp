#include <string.h>
#include <stddef.h>
#include <fstream>
#include <zconf.h>
#include <fcntl.h>
#include "sm_manager.h"
#include "sm_error.h"
#include "rm_filescan.h"
#include "printer.h"


static DataAttrInfo _attrs[MAXATTRS];
static char _buffer[1024];
//
// getline - 从文件中读取一行数据
//
static char* getline(int fd)
{
    int count = 0;
    for ( ; ; ) {
        int n = read(fd, &_buffer[count], 1);
        if (n == 0) {
            if (count == 0) return NULL;
            else break;
        }
        if (_buffer[count] == '\n') {
            _buffer[count] = '\0';
            break;
        }
        count++;
    }
    return _buffer;
}

//
// getitem - 获取一行中的一项
//
static char* getitem(char* &line)
{
    int i;
    char* ptr = line;
    for (i = 0; line[i] != ',' && line[i] != '\0'; i++);
    line[i] = '\0';
    line += i + 1;
    return ptr;
}

//
// openDb - 打开数据库
//
RC SMManager::openDb(const char* dbpath)
{
    if (dbpath == NULL)
        return SM_BADOPEN;
    getcwd(workdir_, 1024);
    chdir(dbpath);
    rmm.openFile("attrcat", attr_);
    rmm.openFile("relcat", rel_);
    opened_ = true;
    return 0;
}

//
// closeDb - 关闭数据库
//
RC SMManager::closeDb()
{
    if (!opened_)
        return SM_NOTOPEN;
    rmm.closeFile(attr_);
    rmm.closeFile(rel_);
    chdir(workdir_);
    opened_ = false;
    return 0;
}

//
// createTable - 构建表
//
RC SMManager::createTable(const char *relname, int count, AttrInfo *infos)
{
    if (count <= 0 || infos == NULL)
        return SM_BADTABLE;

    if ((strcmp(relname, "relcat") == 0) || (strcmp(relname, "attrcat") == 0))
        return SM_BADTABLE;

    int offset = 0;
    RID rid;
    DataAttrInfo attr;
    strcpy(attr.relName, relname);
    for (int i = 0; i < count; i++)
    {
        attr.attrType = infos[i].attrType;
        attr.attrLength = infos[i].attrLength;
        attr.indexNo = -1;			// 在没有正式建立索引之前,idxno都是-1
        attr.offset = offset;
        strcpy(attr.attrName, infos[i].attrName);
        offset += infos[i].attrLength;
        attr_.insertRcd(reinterpret_cast<char*>(&attr), rid);//在元数据中记录属性数据
    }

    rmm.createFile(relname, offset);	// 为表构建一个文件,记录长度为所有属性长度之和
    DataRelInfo rel;
    strcpy(rel.relName, relname);
    rel.attrCount = count;//表中属性个数
    rel.numPages = 1;			// 表所占页的数目
    rel.numRecords = 0;			// 记录的数目
    rel.recordSize = offset; //记录的长度
    rel_.insertRcd(reinterpret_cast<char*>(&rel), rid);//在元数据中记录表数据
    return 0;
}

//删除表
RC SMManager::dropTable(const char *relname)
{
    if (strcmp(relname, "relcat") == 0 || strcmp(relname, "attrcat") == 0)
        return SM_BADTABLE;

    RMFileScan scan;
    RMRecord rcd;
    char* val = const_cast<char*>(relname);//表的名字
    //删除表
    RC errval = scan.openScan(rel_, STRING, MAXNAME + 1, offsetof(DataRelInfo, relName), EQ_OP, val);
    bool found = false;
    //先找元数据里有没有这张表
    while(errval != RM_EOF)
    {
        errval = scan.getNextRcd(rcd);
        if (errval == RM_EOF) break;
        char* addr = NULL;
        rcd.GetData(addr);
        if (strcmp(reinterpret_cast<DataRelInfo *>(addr)->relName, relname) == 0)
        {
            found = true;
            break;
        }
    }
    scan.closeScan();
    if (!found) return SM_NOSUCHTABLE;
    //找到了，删除这张表
    rmm.destroyFile(relname);
    //从元数据中删除
    RID temp;
    rcd.GetRid(temp);
    rel_.deleteRcd(temp);

    // 接下来删除索引
    RMFileScan scan2;
    RMRecord rcd2;
    DataAttrInfo *attr;
    scan2.openScan(attr_, STRING, MAXNAME + 1, offsetof(DataAttrInfo, relName), EQ_OP, val);
    for ( ; ; ) {
        errval = scan2.getNextRcd(rcd2);
        if (errval == RM_EOF) break;
        char* addr = NULL;
        rcd2.GetData(addr);
        attr = reinterpret_cast<DataAttrInfo *>(addr);
        if (attr->indexNo != -1) {
            dropIndex(relname, attr->attrName);
        }
        RID temp;
        rcd2.GetRid(temp);
        //从元数据中删除
        attr_.deleteRcd(temp);
    }
    scan2.closeScan();
    return 0;
}

//
// createIndex - 为表中的某个属性构建一个索引
//
RC SMManager::createIndex(const char* relname, const char* attrname)
{
    DataAttrInfo attr;
    RID rid;
    RC errval = lookupAttr(relname, attrname, attr, rid);//在表中找到该属性
    if (errval != 0) return errval;
    if (attr.indexNo != -1) return SM_INDEXEXISTS;	// 索引已经存在了
    attr.indexNo = attr.offset;//设定索引号
    //创建索引
    ixm.CreateIndex(relname, attr.indexNo, attr.attrType, attr.attrLength);
    RMRecord rcd;
    IXIndexHandle index;
    RMFileHandle file;
    rcd.set(reinterpret_cast<char*>(&attr), sizeof(DataAttrInfo), rid);
    attr_.updateRcd(rcd);//索引号改变了，更新元数据
    //打开索引
    ixm.OpenIndex(relname, attr.indexNo, index);
    //打开表所在的文件
    rmm.openFile(relname, file);
    RMFileScan scan;
    char* addr;
    //遍历表里面所有的内容
    scan.openScan(file, attr.attrType, attr.attrLength, attr.offset, NO_OP, NULL);
    for (; ; ) {
        RMRecord rcd;
        errval = scan.getNextRcd(rcd);
        if (errval == RM_EOF)
            break;
        if (errval != 0) return errval;
        rcd.GetData(addr);
        RID temp;
        rcd.GetRid(temp);
        //把该项属性值和rid插入到索引里
        index.InsertEntry(addr + attr.offset, temp);
    }

    scan.closeScan();
    ixm.CloseIndex(index);
    rmm.closeFile(file);
    return 0;
}

//
// dropIndex - 删除某个属性上的索引
//
RC SMManager::dropIndex(const char *relname, const char *attrname)
{
    DataAttrInfo attr;
    RID rid;
    RC errval = lookupAttr(relname, attrname, attr, rid);
    if (errval != 0) return errval;
    if (attr.indexNo == -1) return SM_INDEXNOTEXISTS;
    ixm.DestroyIndex(relname, attr.indexNo);
    attr.indexNo = -1;
    //索引号改变了，要更新
    RMRecord rcd;
    rcd.set(reinterpret_cast<char*>(&attr), sizeof(DataAttrInfo), rid);
    attr_.updateRcd(rcd);
    return 0;
}


RC SMManager::FindRelForAttr(RelAttr& ra, int nRelations, char * possibleRelations[])
{
    if(ra.relname != NULL) return 0;
    DataAttrInfo a;
    RID rid;
    bool found = false;
    for( int i = 0; i < nRelations; i++ ) {
        RC rc = lookupAttr(possibleRelations[i], ra.attrname, a, rid);
        if(rc == 0 && !found) {
            found = true;
            ra.relname = strdup(possibleRelations[i]);
        }
    }
    if (!found)
        return SM_NOSUCHENTRY;
    else
        return 0;

}

RC SMManager::FindRelForAttr(RelAttr& ra, const char * possibleRelations)
{
    if(ra.relname != NULL) return 0;
    DataAttrInfo a;
    RID rid;
    bool found = false;

    RC rc = lookupAttr(possibleRelations, ra.attrname, a, rid);
    if(rc == 0 && !found) {
        found = true;
        ra.relname = strdup(possibleRelations);
        }

    if (!found)
        return SM_NOSUCHENTRY;
    else
        return 0;

}

//从表中找到相应的属性
RC SMManager::lookupAttr(const char* relname, const char* attrname, DataAttrInfo& attr, RID &rid)
{
    RMFileScan scan;
    RMRecord rcd;
    DataAttrInfo* attrtemp = NULL;
    char* val = const_cast<char*>(relname);
    //在元数据中找属性
    RC errval = scan.openScan(attr_, STRING, MAXNAME + 1, offsetof(DataAttrInfo, relName), EQ_OP, val);

    bool exists = false;
    for (;;) {
        errval = scan.getNextRcd(rcd);
        if (errval == RM_EOF) break;
        char* addr;
        rcd.GetData(addr);
        attrtemp = reinterpret_cast<DataAttrInfo *> (addr);
        if (strcmp(attrtemp->attrName, attrname) == 0) {
            exists = true;
            attr = *attrtemp;
            rcd.GetRid(rid);
            break;
        }
    }
    scan.closeScan();
    if (!exists) return SM_BADATTR;
    return 0;
}

//
// lookupRel - 在relcat表中查询有关于relname这张表的相关信息
//
RC SMManager::lookupRel(const char * relname, DataRelInfo & rel, RID & rid)
{
    RMFileScan scan;
    scan.openScan(rel_, STRING, MAXNAME + 1, offsetof(DataRelInfo, relName), EQ_OP, const_cast<char *>(relname));
    RMRecord rcd;
    RC errval;
    errval = scan.getNextRcd(rcd);
    if (errval == RM_EOF) return SM_NOSUCHTABLE;
    char* temp = NULL;
    rcd.GetData(temp);
    DataRelInfo *addr = reinterpret_cast<DataRelInfo *>(temp);
    rel = *addr;
    rcd.GetRid(rid);
    scan.closeScan();
    return 0;
}

//
// lookupAttrs - 查询某张表所有的属性信息
//
RC SMManager::lookupAttrs(const char* relname, int& nattrs, DataAttrInfo attrs[])
{
    RC errval;
    RMFileScan scan;
    char* val = const_cast<char *>(relname);
    DataAttrInfo* attr;
    char* temp = NULL;
    scan.openScan(attr_, STRING, MAXNAME + 1, offsetof(DataRelInfo, relName), EQ_OP, val);
    RMRecord rcd;
    nattrs = 0;
    for (;;) {
        errval = scan.getNextRcd(rcd);
        if (errval == RM_EOF) break;
        rcd.GetData(temp);
        attr = reinterpret_cast<DataAttrInfo *>(temp);
        attrs[nattrs] = *attr;
        nattrs++;
    }
    scan.closeScan();
    return 0;
}

//把文件中的数据插入表中
RC SMManager::load(const char *relname, const char *filename)
{
    RMFileHandle file;
    RC errval = 0;
    char cache[MAXSTRINGLEN];
    int nattrs = 0, nlines = 0;
    char* line;
    rmm.openFile(relname, file);
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return SM_BADTABLE;
    //找到这张表上的所有属性信息
    errval = lookupAttrs(relname, nattrs, _attrs);
    if (errval < 0) return errval;

    // 打开所有的索引,因为可能要同时插入索引
    std::vector<IXIndexHandle> indexes(nattrs);
    int offset = 0;
    int count = 0;
    for (int i = 0; i < nattrs; i++) {
        offset += _attrs[i].attrLength;
        if (_attrs[i].indexNo != -1) {
            count++;
            ixm.OpenIndex(relname, _attrs[i].indexNo, indexes[i]);
        }
    }
    //把文件中的数据读到cache里
    while ((line = getline(fd)) != NULL) {
        char* item;
        for (int i = 0; i < nattrs; i++) {
            item = getitem(line);
            switch (_attrs[i].attrType)
            {
                case INT:
                {
                    int val;
                    sscanf(item, "%d", &val);
                    memcpy(cache + _attrs[i].offset, &val, _attrs[i].attrLength);
                    break;
                }
                case FLOAT:
                {
                    float val;
                    sscanf(item, "%f", &val);
                    memcpy(cache + _attrs[i].offset, &val, _attrs[i].attrLength);
                    break;
                }
                case STRING:
                {
                    memcpy(cache + _attrs[i].offset, item, _attrs[i].attrLength);
                    break;
                }
                default:
                    break;

            }
        }
        RID rid;
        //插到表中
        file.insertRcd(cache, rid);
        //有索引的插到索引中
        for (int i = 0; i < count; i++)
        {
            indexes[i].InsertEntry(cache + _attrs[i].offset, rid);
        }
        nlines++;
    }

    DataRelInfo rel;
    RID rid;
    lookupRel(relname, rel, rid);
    //更新元数据中的数据行数和
    rel.numRecords += nlines;
    rel.numPages = file.pagesSize();
    RMRecord rcd;
    rcd.set((char*)&rel, sizeof(DataRelInfo), rid);
    rel_.updateRcd(rcd);
    rmm.closeFile(file);
    for (int i = 0; i < count; i++) {
        ixm.CloseIndex(indexes[i]);
    }
    close(fd);
    return 0;
}

//从表中找到所有属性
RC SMManager::loadFromTable(const char* relname, int& count, DataAttrInfo* &attrs)
{
    if (relname == NULL) return SM_NOSUCHTABLE;
    RMFileScan scan;
    //在元数据中找到这张表
    RC errval = scan.openScan(rel_, STRING, MAXNAME + 1, offsetof(DataAttrInfo, relName), EQ_OP, const_cast<char *>(relname));
    RMRecord rcd;
    scan.getNextRcd(rcd);
    if (errval == RM_EOF) return SM_NOSUCHTABLE;
    char *temp = NULL;
    rcd.GetData(temp);
    DataRelInfo *rels = reinterpret_cast<DataRelInfo *> (temp);
    scan.closeScan();
    count = rels->attrCount;
    attrs = new DataAttrInfo[count];
    //找到该表所有属性
    RMFileScan s2;
    s2.openScan(attr_, STRING, MAXNAME + 1, offsetof(DataAttrInfo, relName), EQ_OP, const_cast<char *>(relname));

    int i;
    for (i = 0; i < count; i++) {
        RMRecord rcd;
        errval = s2.getNextRcd(rcd);
        if (errval == RM_EOF) break;
        char* temp;
        rcd.GetData(temp);
        attrs[i] = *reinterpret_cast<DataAttrInfo*>(temp);
    }

    if (i < count) return SM_BADTABLE;
    s2.closeScan();
    return 0;
}

//
// help - 输出所有的表
//
RC SMManager::help()
{
    RMRecord rcd;
    char* addr;
    DataAttrInfo attr;
    strcpy(attr.relName, "XXXX");
    strcpy(attr.attrName, "table name");
    attr.offset = offsetof(DataRelInfo, relName);
    attr.attrType = STRING;
    attr.attrLength = MAXNAME + 1;

    Printer p(&attr, 1); // 只需要输出所有的表的名称即可
    p.printHeader();
    RMFileScan scan;
    RC errval = scan.openScan(rel_, INT, sizeof(int), 0, NO_OP, NULL);

    for (;;) {
        errval = scan.getNextRcd(rcd);
        if (errval == RM_EOF) break;
        char* addr = NULL;
        rcd.GetData(addr);
        p.print(addr);
    }
    p.printFooter();
    scan.closeScan();
    std::cout<<rel_.pffile_->buff<<std::endl;
    return 0;
}

//
// help - 给定表的名称,输出这张表中所有的属性
//
RC SMManager::help(char *relname)
{
    int nattrs;
    lookupAttrs(relname, nattrs, _attrs); // 获取attrcat这张表的所有属性
    std::cout<<"attrname    "<<"attrtype    "<<"attrlen    "<<std::endl;
    for(int i =0; i<36; ++i )
    {
        std::cout<<"-";
    }
    std::cout<<std::endl;

    for(int i = 0; i<nattrs; ++i)
    {
        std::cout<<_attrs[i].attrName;
        for(int i =0;i<10;++i)
        {
            std::cout<<" ";
        }
        std::cout<<_attrs[i].attrLength;
        for(int i =0;i<10;++i)
        {
            std::cout<<" ";
        }
        if(_attrs[i].attrType == STRING)
        {
            std::cout<<"string";
            for(int i =0;i<10;++i)
            {
                std::cout<<" ";
            }
        }
        else if(_attrs[i].attrType == INT)
        {
            std::cout<<"int";
            for(int i =0;i<10;++i)
            {
                std::cout<<" ";
            }
        }
        else
        {
            std::cout<<"float";
            for(int i =0;i<10;++i)
            {
                std::cout<<" ";
            }
        }
        std::cout<<std::endl;

    }

    return 0;
}

//
// print - 输出指定表单的所有记录
//
RC SMManager::print(const char *relname)
{
    RMFileHandle file;
    int nattrs;
    RC errval = rmm.openFile(relname, file);
    lookupAttrs(relname, nattrs, _attrs);
    Printer print(_attrs, nattrs);
    print.printHeader();
    RMFileScan scan;
    scan.openScan(file, INT, sizeof(int), 0, NO_OP, NULL);
    RMRecord rcd;
    char* data;
    for (;;) {
        errval = scan.getNextRcd(rcd);
        if (errval == RM_EOF) break;
        rcd.GetData(data);
        print.print(data);
    }
    print.printFooter();
    scan.closeScan();
    return 0;
}
