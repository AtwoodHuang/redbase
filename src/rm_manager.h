#ifndef REDBASE_RM_MANAGER_H
#define REDBASE_RM_MANAGER_H

#include "common_thing.h"
#include "pf_manager.h"
#include "pf_error.h"
#include "rm_error.h"
#include "rm_filehandle.h"

class RMManager {
public:
    RMManager(PFManager &pfm_):pfm(pfm_) {};
    ~RMManager() {};
public:
    RC createFile(const char* pathname, unsigned int recordsize);
    RC destroyFile(const char* pathname);
    RC openFile(const char* pathname, RMFileHandle &rmfile);
    RC closeFile(RMFileHandle& rmfile);

private:
    PFManager& pfm;
};


#endif //REDBASE_RM_MANAGER_H
