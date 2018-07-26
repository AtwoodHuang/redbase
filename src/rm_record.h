#ifndef REDBASE_RM_RECORD_H
#define REDBASE_RM_RECORD_H

#include "common_thing.h"
#include "rm_rid.h"

class RMRecord {
public:
    RMRecord();
    ~RMRecord();

    RC GetData(char* &pData) const;

    RC set(char* pData, int size, RID id);

    RC GetRid(RID &rid)const;

private:

    int recordsize;
    char* data;
    RID rid;

};


#endif //REDBASE_RM_RECORD_H
