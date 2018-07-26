#ifndef REDBASE_IX_MANAGER_H
#define REDBASE_IX_MANAGER_H

#include "common_thing.h"
#include "pf_manager.h"
#include "ix_indexhandle.h"

class IXManager {
public:
    IXManager(PFManager &pfm);

    ~IXManager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength,
                   int pageSize = PF_PAGE_SIZE);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo, IXIndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IXIndexHandle &indexHandle);

private:
    PFManager &pfm;
};


#endif //REDBASE_IX_MANAGER_H
