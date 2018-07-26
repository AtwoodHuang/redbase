#ifndef SM_MANAGER
#define SM_MANAGER
#include "ix_manager.h"
#include "rm_manager.h"
#include "data_attr.h"
#include "parse.h"


class SMManager {
    friend class QLManager;
public:
    SMManager(IXManager &ixm_, RMManager &rmm_) : rmm(rmm_), ixm(ixm_),opened_(false) {}
    ~SMManager() {}
public:
    RC openDb(const char* dbname);
    RC closeDb();
    RC createTable(const char *relname, int count, AttrInfo  *infos);
    RC dropTable(const char *relname);           // Destroy relname
    RC createIndex(const char *relname, const char *attrname);
    RC dropIndex(const char *relname, const char *attrname);
    RC load(const char *relname, const char *filename);
    RC help();                                   // Help for database
    RC help(char *relname);						// Help for relname
    RC print(const char *relname);               // Print relname
public:
    RC lookupAttr(const char* relname, const char* attrname, DataAttrInfo& attr, RID &rid);
    RC lookupAttrs(const char* relname, int& nattrs, DataAttrInfo attrs[]);
    RC lookupRel(const char* relname, DataRelInfo &rel, RID &rid);
    //找到属性对应的表
    RC FindRelForAttr(RelAttr& ra, int nRelations, char *possibleRelations[]);
    RC FindRelForAttr(RelAttr& ra, const char * possibleRelations);
private:
    RC loadFromTable(const char* relname, int& count, DataAttrInfo* &attrs);
private:
    RMManager& rmm;
    IXManager& ixm;
    RMFileHandle rel_; //元数据，表记录
    RMFileHandle attr_;//元数据，属性记录
    char workdir_[1024];
    bool opened_;		// 是否已经打开
};

#endif /* SM_MANAGER */