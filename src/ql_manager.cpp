#include <algorithm>
#include "ql_manager.h"
#include "ql_error.h"
#include <vector>
#include "ix_indexscan.h"
#include "ix_error.h"
#include "comp.h"
#include "printer.h"
#include "rm_filescan.h"
using namespace std;

#define MAXDATAATTR		(3 * MAXATTRS)
#define MAXCONDITIONS	(2 * MAXATTRS)
#define MAXCHAR			1024


static char buffer[1024];

static DataAttrInfo attrs[MAXDATAATTR];

//
// insert - 将数据插入到relname这张表中
//
RC QLManager::insert(const char* relname, int nvals, Value vals[])
{
    int nattrs;
    //插入的元素数和表中的元素数是否一致
    smm.lookupAttrs(relname, nattrs, attrs);
    if (nattrs != nvals) {
        return QL_INVALIDSIZE;
    }

    for (int i = 0; i < nvals; i++)
    {
        if (vals[i].type != attrs[i].attrType) return QL_JOINKEYTYPEMISMATCH;
    }

    int offset = 0;
    memset(buffer, 0, 1024);//字符串比较时是固定长度比较，为了保险起见，先把buff全设为0
    for (int i = 0; i < nvals; i++) {
        memcpy(buffer + offset, vals[i].data, attrs[i].attrLength);
        offset += attrs[i].attrLength;
    }
    RMFileHandle file;
    RID rid;
    rmm.openFile(relname, file);
    file.insertRcd(buffer, rid);
    rmm.closeFile(file);

    //有索引，就在索引中插入
    IXIndexHandle handle;
    for (int i = 0; i < nvals; i++)
    {
        if(attrs[i].indexNo != -1)
        {
            ixm.OpenIndex(relname, attrs[i].indexNo, handle);
            handle.InsertEntry(buffer + attrs[i].offset, rid);
            ixm.CloseIndex(handle);
        }
    }

    //记录数增加了，更新元数据中的数据
    DataRelInfo reladd;
    RID ridadd;
    smm.lookupRel(relname, reladd, ridadd);
    reladd.numRecords += 1;
    RMRecord rcdadd;
    rcdadd.set((char*)&reladd, sizeof(DataRelInfo), ridadd);
    smm.rel_.updateRcd(rcdadd);
    return 0;
}

//
// select : 解析select命令,暂时只支持单表查询
//
RC QLManager::select (int nSelAttrs,
           const RelAttr selAttrs_[],
           int           nRelations,
           const char * const relations_[],
           int           nConditions,
           const Condition conditions_[])
{
    //要查询的属性
    RelAttr* selAttrs = new RelAttr[nSelAttrs];
    for (int i = 0; i < nSelAttrs; i++) {
        selAttrs[i].relname = selAttrs_[i].relname;//表名有可能为NULL，这里先不分配空间
        selAttrs[i].attrname = strdup(selAttrs_[i].attrname);
    }

    //要查询的表
    char** relations = new char*[nRelations];
    for (int i = 0; i < nRelations; i++) {
        relations[i] = strdup(relations_[i]);
    }

    //关系
    Condition* conditions = new Condition[nConditions];
    for (int i = 0; i < nConditions; i++) {
        conditions[i] = conditions_[i];
    }

    //select *
    if(nSelAttrs == 1 && strcmp(selAttrs[0].attrname, "*") == 0) {

        //先把之前分配的空间删掉
        for (int i = 0; i < nSelAttrs; i++) {
            free(selAttrs[i].attrname);
        }
        delete [] selAttrs;

        //找到所有属性
        nSelAttrs = 0;
        for (int i = 0; i < nRelations; i++) {
            int count;
            RC rc = smm.lookupAttrs(relations[i], count, attrs);
            if (rc != 0) return rc;
            nSelAttrs += count;
        }


        selAttrs = new RelAttr[nSelAttrs];

        int j = 0;
        for (int i = 0; i < nRelations; i++) {
            int count;
            RC rc = smm.lookupAttrs(relations[i], count, attrs);
            if (rc != 0) return rc;
            for (int k = 0; k < count; k++) {
                selAttrs[j].attrname = strdup(attrs[k].attrName);
                selAttrs[j].relname = strdup(relations[i]);
                j++;
            }
        }
    }

    //处理表名为NULL的情况
    for (int i = 0; i < nSelAttrs; i++) {
        if(selAttrs[i].relname == NULL)
        {
            RC rc = smm.FindRelForAttr(selAttrs[i], nRelations, relations);
            if (rc != 0) return rc;
        } else {
            selAttrs[i].relname = strdup(selAttrs[i].relname);
        }
    }

    for (int i = 0; i < nConditions; i++) {
        if(conditions[i].lhsAttr.relname == NULL) {
            RC rc = smm.FindRelForAttr(conditions[i].lhsAttr, nRelations, relations);
            if (rc != 0) return rc;
        } else {
            conditions[i].lhsAttr.relname = strdup(conditions[i].lhsAttr.relname);
        }

        if(conditions[i].bRhsIsAttr == true) {
            if(conditions[i].rhsAttr.relname == NULL) {
                RC rc = smm.FindRelForAttr(conditions[i].rhsAttr, nRelations, relations);
                if (rc != 0) return rc;
            } else {
                conditions[i].rhsAttr.relname = strdup(conditions[i].rhsAttr.relname);
            }
        }
    }


    //确保condition里的表名都在from里
    for (int i = 0; i < nConditions; i++) {
        bool lfound = false;
        for (int j = 0; j < nRelations; j++) {
            if(strcmp(conditions[i].lhsAttr.relname, relations[j]) == 0) {
                lfound = true;
                break;
            }
        }
        if(!lfound)
            return QL_RELMISSINGFROMFROM;

        if(conditions[i].bRhsIsAttr == true) {
            bool rfound = false;
            for (int j = 0; j < nRelations; j++) {
                if(strcmp(conditions[i].rhsAttr.relname, relations[j]) == 0) {
                    rfound = true;
                    break;
                }
            }
            if(!rfound)
                return QL_RELMISSINGFROMFROM;
        }
    }

    if(nConditions > 1)
    {
        std::cout<<"only support one relation now"<<std::endl;
        return 0;
    }

    //一元条件
    std::vector<Condition> one;
    //二元条件
    std::vector<Condition> two;

    for(int i = 0; i<nConditions; ++i)
    {
        if(conditions[i].bRhsIsAttr == false)
        {
            one.push_back(conditions[i]);
        }
        else
        {
            two.push_back(conditions[i]);
        }

    }

    //先在一元条件中找有索引的
    Condition indexcondition;
    DataAttrInfo indexattr;
    bool haveindex = false;
    for(auto a = one.begin(); a != one.end(); ++a)
    {
        DataAttrInfo temp;
        RID rid;
        smm.lookupAttr(relations[0],a->lhsAttr.attrname, temp, rid);
        if(temp.indexNo != -1)
        {
            indexcondition = *a;
            haveindex = true;
            //把有索引的项删除
            one.erase(a);
            indexattr = temp;
            break;
        }

    }

    //有索引
    if(haveindex == true)
    {
        IXIndexScan scan;
        IXIndexHandle handle;
        //打开索引
        ixm.OpenIndex(relations[0], indexattr.indexNo, handle);
        scan.openScan(handle, indexcondition.op, indexcondition.rhsValue.data);
        //打开记录的表
        RMFileHandle rmhandle;
        rmm.openFile(relations[0], rmhandle);
        RC errval = OK_RC;
        DataAttrInfo* printAttrs = new DataAttrInfo[nSelAttrs];
        for(int i = 0; i< nSelAttrs; ++i)
        {
            RID temp;
            smm.lookupAttr(selAttrs[i].relname, selAttrs[i].attrname, printAttrs[i], temp);
        }
        Printer print(printAttrs, nSelAttrs);
        //输出头部
        print.printHeader();
        while(true)
        {
            RID rid;
            RMRecord record;
            errval = scan.getNextEntry(rid);
            if (errval == IX_EOF) break;
            rmhandle.getRcd(rid, record);
            //判断剩下的一元条件和二元条件
            if(oneattrs(one, record) && twoattrs(two, record))
            {
                //条件全都满足
                //输出数据
                char* data = NULL;
                record.GetData(data);
                print.print(data);
                //输出尾部

            }
        }
        print.printFooter();
        delete []printAttrs;
    }
    else
    {
        //没有索引,直接遍历相应的rmfile即可
        RMFileHandle rmhandle;
        rmm.openFile(relations[0], rmhandle);
        RMFileScan scan;
        //所有的内容都要遍历，所以后5个参数随便填就行
        scan.openScan(rmhandle,INT, 0, 0, NO_OP, NULL);
        RC errval = OK_RC;
        DataAttrInfo* printAttrs = new DataAttrInfo[nSelAttrs];
        for(int i = 0; i< nSelAttrs; ++i)
        {
            RID temp;
            smm.lookupAttr(selAttrs[i].relname, selAttrs[i].attrname, printAttrs[i], temp);
        }
        Printer print(printAttrs, nSelAttrs);
        //输出头部
        print.printHeader();
        while(errval != RM_EOF)
        {
            RMRecord record;
            errval = scan.getNextRcd(record);
            if (errval == RM_EOF) break;
            if(oneattrs(one, record) && twoattrs(two, record))
            {
                //条件全都满足

                //输出数据
                char* data = NULL;
                record.GetData(data);
                print.print(data);


            }
        }
        //输出尾部
        print.printFooter();
        delete []printAttrs;
    }


    //资源释放
    for(int i = 0; i < nSelAttrs; i++) {
        if(selAttrs[i].attrname) {
            free(selAttrs[i].attrname);
        }
        if(selAttrs[i].relname) {
            free(selAttrs[i].relname);
        }
    }
    delete [] selAttrs;

    for (int i = 0; i < nRelations; i++) {
        free(relations[i]);
    }
    delete [] relations;

    for(int i = 0; i < nConditions; i++) {
        free(conditions[i].lhsAttr.relname);
        if(conditions[i].bRhsIsAttr == true)
            free(conditions[i].rhsAttr.relname);
    }
    delete [] conditions;

    return 0;
}


//判断二元条件是否成立
bool QLManager::twoattrs(std::vector<Condition> two, const RMRecord &record)
{
    for(auto a :two)
    {
        DataAttrInfo left;
        DataAttrInfo right;
        RID ridf;
        RID ridr;
        smm.lookupAttr(a.lhsAttr.relname, a.lhsAttr.attrname, left, ridf);
        smm.lookupAttr(a.rhsAttr.relname, a.rhsAttr.attrname, right, ridr);
        Comp* comp = make_comp(left.attrType, left.attrLength);
        char* data = NULL;
        record.GetData(data);
        if(!comp->eval(data + left.offset, a.op, data+right.offset))
            return false;
    }
    return true;

}



bool QLManager::oneattrs(std::vector<Condition> one, const RMRecord &record)
{
    for(auto a :one)
    {
        DataAttrInfo left;
        RID ridf;
        smm.lookupAttr(a.lhsAttr.relname, a.lhsAttr.attrname, left, ridf);
        Comp* comp = make_comp(left.attrType, left.attrLength);
        char* data = NULL;
        record.GetData(data);
        if(!comp->eval(data + left.offset, a.op, a.rhsValue.data))
            return false;
    }
    return true;
}




RC QLManager::Delete (const char *relName, int nConditions, const Condition conditions_[])
{

    //删除的条件
    Condition* conditions = new Condition[nConditions];
    for (int i = 0; i < nConditions; i++) {
        conditions[i] = conditions_[i];
    }


    //处理表名为NULL的情况

    for (int i = 0; i < nConditions; i++) {
        if(conditions[i].lhsAttr.relname == NULL) {
            RC rc = smm.FindRelForAttr(conditions[i].lhsAttr, relName);
            if (rc != 0) return rc;
        } else {
            conditions[i].lhsAttr.relname = strdup(conditions[i].lhsAttr.relname);
        }

        if(conditions[i].bRhsIsAttr == true) {
            if(conditions[i].rhsAttr.relname == NULL) {
                RC rc = smm.FindRelForAttr(conditions[i].rhsAttr, relName);
                if (rc != 0) return rc;
            } else {
                conditions[i].rhsAttr.relname = strdup(conditions[i].rhsAttr.relname);
            }
        }
    }


    //一元条件
    std::vector<Condition> one;
    //二元条件
    std::vector<Condition> two;

    for(int i = 0; i<nConditions; ++i)
    {
        if(conditions[i].bRhsIsAttr == false)
        {
            one.push_back(conditions[i]);
        }
        else
        {
            two.push_back(conditions[i]);
        }

    }

    //先在一元条件中找有索引的
    Condition indexcondition;
    DataAttrInfo indexattr;
    bool haveindex = false;
    for(auto a = one.begin(); a != one.end(); ++a)
    {
        DataAttrInfo temp;
        RID rid;
        smm.lookupAttr(relName,a->lhsAttr.attrname, temp, rid);
        if(temp.indexNo != -1)
        {
            indexcondition = *a;
            haveindex = true;
            //把有索引的项删除
            one.erase(a);
            indexattr = temp;
            break;
        }

    }

    //有索引
    if(haveindex == true)
    {
        IXIndexScan scan;
        IXIndexHandle handle;
        //打开索引
        ixm.OpenIndex(relName, indexattr.indexNo, handle);
        scan.openScan(handle, indexcondition.op, indexcondition.rhsValue.data);
        //打开记录的表
        RMFileHandle rmhandle;
        rmm.openFile(relName, rmhandle);
        RC errval = OK_RC;
        while(errval != IX_EOF)
        {
            RID rid;
            RMRecord record;
            errval = scan.getNextEntry(rid);
            if (errval == IX_EOF) break;
            rmhandle.getRcd(rid, record);
            //判断剩下的一元条件和二元条件
            if(oneattrs(one, record) && twoattrs(two, record))
            {
                //条件全都满足, 先删索引，再删记录
                char* data = NULL;
                record.GetData(data);
                RID temp;
                record.GetRid(temp);
                handle.DeleteEntry(data + indexattr.offset, temp);
                rmhandle.deleteRcd(temp);
            }
        }
    }
    else
    {
        //没有索引,直接遍历相应的rmfile即可
        RMFileHandle rmhandle;
        rmm.openFile(relName, rmhandle);
        RMFileScan scan;
        //所有的内容都要遍历，所以后5个参数随便填就行
        scan.openScan(rmhandle,INT, 0, 0, NO_OP, NULL);
        RC errval = OK_RC;
        while(errval != RM_EOF)
        {
            RMRecord record;
            errval = scan.getNextRcd(record);
            if (errval == RM_EOF) break;
            if(oneattrs(one, record) && twoattrs(two, record))
            {
                //条件全都满足删记录
                RID rid;
                record.GetRid(rid);
                rmhandle.deleteRcd(rid);
            }
        }
    }

    //记录数减少了，更新元数据中的数据
    DataRelInfo relsub;
    RID ridsub;
    smm.lookupRel(relName, relsub, ridsub);
    relsub.numRecords -= 1;
    RMRecord rcdsub;
    rcdsub.set((char*)&relsub, sizeof(DataRelInfo), ridsub);
    smm.rel_.updateRcd(rcdsub);

    //资源释放
    for(int i = 0; i < nConditions; i++) {
        free(conditions[i].lhsAttr.relname);
        if(conditions[i].bRhsIsAttr == true)
            free(conditions[i].rhsAttr.relname);
    }
    delete [] conditions;

    return 0;




}


RC QLManager::Update (const char *relName, const RelAttr &updAttr_, const int bIsValue, const RelAttr &rhsRelAttr_, const Value &rhsValue_, int  nConditions, const Condition conditions_[])
{
    RelAttr updateattr = updAttr_;
    updateattr.attrname = strdup(updAttr_.attrname);
    if(updateattr.relname == NULL)
    {
        RC rc = smm.FindRelForAttr(updateattr, relName);
        if (rc != 0) return rc;
    }
    else
    {
        updateattr.relname =  strdup(updAttr_.relname);
    }

    RelAttr rhsRelAttr = rhsRelAttr_;
    if(bIsValue == 0)
    {
        rhsRelAttr.attrname = strdup(rhsRelAttr.attrname);
        if(rhsRelAttr.relname == NULL)
        {
            RC rc = smm.FindRelForAttr(rhsRelAttr, relName);
            if (rc != 0) return rc;
        }
        else
        {
            rhsRelAttr.relname =  strdup(rhsRelAttr_.relname);
        }
    }

    //删除的条件
    Condition* conditions = new Condition[nConditions];
    for (int i = 0; i < nConditions; i++) {
        conditions[i] = conditions_[i];
    }


    //处理表名为NULL的情况

    for (int i = 0; i < nConditions; i++) {
        if(conditions[i].lhsAttr.relname == NULL) {
            RC rc = smm.FindRelForAttr(conditions[i].lhsAttr, relName);
            if (rc != 0) return rc;
        } else {
            conditions[i].lhsAttr.relname = strdup(conditions[i].lhsAttr.relname);
        }

        if(conditions[i].bRhsIsAttr == true) {
            if(conditions[i].rhsAttr.relname == NULL) {
                RC rc = smm.FindRelForAttr(conditions[i].rhsAttr, relName);
                if (rc != 0) return rc;
            } else {
                conditions[i].rhsAttr.relname = strdup(conditions[i].rhsAttr.relname);
            }
        }
    }

    //一元条件
    std::vector<Condition> one;
    //二元条件
    std::vector<Condition> two;

    for(int i = 0; i<nConditions; ++i)
    {
        if(conditions[i].bRhsIsAttr == false)
        {
            one.push_back(conditions[i]);
        }
        else
        {
            two.push_back(conditions[i]);
        }

    }

    //先在一元条件中找有索引的
    Condition indexcondition;
    DataAttrInfo indexattr;
    bool haveindex = false;
    for(auto a = one.begin(); a != one.end(); ++a)
    {
        DataAttrInfo temp;
        RID rid;
        smm.lookupAttr(relName,a->lhsAttr.attrname, temp, rid);
        if(temp.indexNo != -1)
        {
            indexcondition = *a;
            haveindex = true;
            //把有索引的项删除
            one.erase(a);
            indexattr = temp;
            break;
        }

    }

    //有索引
    if(haveindex == true)
    {
        IXIndexScan scan;
        IXIndexHandle handle;
        //打开索引
        ixm.OpenIndex(relName, indexattr.indexNo, handle);
        scan.openScan(handle, indexcondition.op, indexcondition.rhsValue.data);
        //打开记录的表
        RMFileHandle rmhandle;
        rmm.openFile(relName, rmhandle);
        RC errval = OK_RC;
        while(errval != IX_EOF)
        {
            RID rid;
            RMRecord record;
            errval = scan.getNextEntry(rid);
            if (errval == IX_EOF) break;
            rmhandle.getRcd(rid, record);
            //判断剩下的一元条件和二元条件
            if(oneattrs(one, record) && twoattrs(two, record))
            {
                //条件全都满足,更新
                //右边是属性
                if(bIsValue == 0)
                {
                    DataAttrInfo left;
                    DataAttrInfo right;
                    RID temp;
                    smm.lookupAttr(relName, updateattr.attrname, left, temp);
                    smm.lookupAttr(relName, rhsRelAttr.attrname, right, temp);
                    char* data;
                    RID ridtemp;
                    record.GetData(data);
                    record.GetRid(ridtemp);
                    //更新的属性带索引，先删索引
                    if(left.indexNo != -1)
                    {
                        handle.DeleteEntry(data + left.offset, ridtemp);
                    }
                    memcpy(data + left.offset, data+right.offset, left.attrLength);
                    rmhandle.updateRcd(record);
                    //再插入新索引
                    if(left.indexNo != -1)
                    {
                        handle.InsertEntry(data + left.offset, ridtemp);
                    }

                }
                else
                {
                    //右边是数据
                    DataAttrInfo left;
                    RID temp;
                    smm.lookupAttr(relName, updateattr.attrname, left, temp);
                    char* data;
                    RID temprid;
                    record.GetData(data);
                    record.GetRid(temprid);
                    //更新的属性带索引，先删索引
                    if(left.indexNo != -1)
                    {
                        handle.DeleteEntry(data + left.offset, temprid);
                    }
                    memcpy(data + left.offset, rhsValue_.data, left.attrLength);
                    rmhandle.updateRcd(record);
                    //再插入新索引
                    if(left.indexNo != -1)
                    {
                        handle.InsertEntry(data + left.offset, temprid);
                    }
                }

            }
        }
    }
    else
    {
        //没有索引,直接遍历相应的rmfile即可
        RMFileHandle rmhandle;
        rmm.openFile(relName, rmhandle);
        RMFileScan scan;
        //所有的内容都要遍历，所以后5个参数随便填就行
        scan.openScan(rmhandle,INT, 0, 0, NO_OP, NULL);
        RC errval = OK_RC;
        while(errval != RM_EOF)
        {
            RMRecord record;
            errval = scan.getNextRcd(record);
            if (errval == RM_EOF) break;
            if(oneattrs(one, record) && twoattrs(two, record))
            {
                //条件全都满足,更新
                //右边是属性
                if(bIsValue == 0)
                {
                    DataAttrInfo left;
                    DataAttrInfo right;
                    RID temp;
                    smm.lookupAttr(relName, updateattr.attrname, left, temp);
                    smm.lookupAttr(relName, rhsRelAttr.attrname, right, temp);
                    char* data;
                    record.GetData(data);
                    memcpy(data + left.offset, data+right.offset, left.attrLength);
                    rmhandle.updateRcd(record);
                }
                else
                {
                    //右边是数据
                    DataAttrInfo left;
                    RID temp;
                    smm.lookupAttr(relName, updateattr.attrname, left, temp);
                    char* data;
                    record.GetData(data);
                    memcpy(data + left.offset, rhsValue_.data, left.attrLength);
                    rmhandle.updateRcd(record);

                }
            }
        }
    }


    //释放资源
    for(int i = 0; i < nConditions; i++) {
        free(conditions[i].lhsAttr.relname);
        if(conditions[i].bRhsIsAttr == true)
            free(conditions[i].rhsAttr.relname);
    }
    delete [] conditions;

    if(updateattr.attrname)
        free(updateattr.attrname);
    if(updateattr.relname)
        free(updateattr.relname);

    if(bIsValue == 0)
    {
        if(rhsRelAttr.relname)
            free(rhsRelAttr.relname);
        if(rhsRelAttr.attrname)
            free(rhsRelAttr.attrname);
    }

    return 0;



}





