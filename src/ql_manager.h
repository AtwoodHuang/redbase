#ifndef QL_MANAGER_H
#define QL_MANAGER_H
#include <stdlib.h>
#include <string.h>
#include "pf_manager.h"
#include "rm_manager.h"
#include "ql_manager.h"
#include "ix_manager.h"
#include "sm_manager.h"
#include "parse.h"
using namespace std;



class QLManager {
public:
    QLManager(SMManager &smm_, IXManager &ixm_, RMManager &rmm_)
            :rmm(rmm_), ixm(ixm_), smm(smm_)
    {

    }
    ~QLManager() {}
public:
    RC insert(const char* relname, int nvals, Value vals[]);
    RC select (int           nSelAttrs,        // # attrs in Select clause
               const RelAttr selAttrs[],       // attrs in Select clause
               int           nRelations,       // # relations in From clause
               const char * const relations[], // relations in From clause
               int           nConditions,      // # conditions in Where clause
               const Condition conditions[]);
    RC Delete (const char *relName,            // relation to delete from
               int        nConditions,         // # conditions in Where clause
               const Condition conditions[]);  // conditions in Where clause
    RC Update (const char *relName,            // relation to update
               const RelAttr &updAttr,         // attribute to update
               const int bIsValue,             // 0/1 if RHS of = is attribute/value
               const RelAttr &rhsRelAttr,      // attr on RHS of =
               const Value &rhsValue,          // value on RHS of =
               int   nConditions,              // # conditions in Where clause
               const Condition conditions[]);  // conditions in Where clause

private:
    bool twoattrs(std::vector<Condition> two, const RMRecord &record);
    bool oneattrs(std::vector<Condition> one, const RMRecord &record);
    RMManager& rmm;
    IXManager& ixm;
    SMManager& smm;
};

#endif /* QL_MANAGER_H */