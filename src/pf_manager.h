#ifndef REDBASE_PF_MANAGER_H
#define REDBASE_PF_MANAGER_H

#include "common_thing.h"
#include "pf_common.h"
#include "pf_buffer.h"
#include "pf_filehandle.h"
#include "pf_error.h"

class PFManager {
public:
    PFManager() : buff(NULL){buff = new PFBuffer;};
    ~PFManager() {};
    RC createFile(const char *pathname);
    RC destroyFile(const char *pathname);
    RC openFile(const char* pathname, PFFileHandle &file);
    RC closeFile(PFFileHandle &file);
private:
    PFBuffer *buff;
};
#endif //REDBASE_PF_MANAGER_H
