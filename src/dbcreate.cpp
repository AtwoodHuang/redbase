

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include "rm_manager.h"
#include "sm_manager.h"
#include "common_thing.h"


using namespace std;


int main(int argc, char *argv[])
{
    RC rc;

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <dbname> \n";
        exit(1);
    }

    string dbname(argv[1]);
    //string dbname("djf");
    //建立子目录
    stringstream command;
    command << "mkdir " << dbname;
    rc = system (command.str().c_str());
    if(rc != 0) {
        cerr << argv[0] << " mkdir error for " << dbname << "\n";
        exit(rc);
    }

    if (chdir(dbname.c_str()) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    //元数据
    PFManager pfm;
    RMManager rmm(pfm);
    RMFileHandle relation, attrs;

    rmm.createFile("relcat",sizeof(DataRelInfo));
    rmm.createFile("attrcat", sizeof(DataAttrInfo));


    return(0);
}

