#ifndef REDBASE_RBPARSE_H
#define REDBASE_RBPARSE_H

#include <string>
#include "pf_manager.h"
#include "sm_manager.h"
#include "ql_manager.h"

class RBparse {
public:
    void startparse();
    RBparse(PFManager &pf, SMManager &sm, QLManager &ql) :pfm(pf), psm(sm), qlm(ql){}

private:
    std::string buff;
    int cur = 0;
    PFManager& pfm;
    SMManager& psm;
    QLManager& qlm;
    void create();
    void drop();
    void createtable();
    void droptable();
    void createindex();
    void dropindex();
    void load();
    void help();
    void Print();
    void select();
    void insert();
    void Delete();
    void update();
    void passspace();
    void parsecondition(string con, Condition* &condition, int &count);

};


#endif //REDBASE_RBPARSE_H
