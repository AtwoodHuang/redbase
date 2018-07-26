
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "common_thing.h"
#include "rm_manager.h"
#include "sm_manager.h"
#include "ql_manager.h"
#include "parse.h"
#include "RBparse.h"



PFManager pfm;
RMManager rmm(pfm);
IXManager ixm(pfm);
SMManager smm(ixm, rmm);
QLManager qlm(smm, ixm, rmm);


int main(int argc, char *argv[])
{
    char *dbname;
    RC rc;

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    dbname = argv[1];

    if ((rc = smm.openDb(dbname))) {
        return (1);
    }


    RBparse parser(pfm, smm, qlm);
    parser.startparse();

    if ((rc = smm.closeDb())) {
        return (1);
    }


    cout << "Bye.\n";
}