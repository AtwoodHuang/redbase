#ifndef REDBASE_XX_H
#define REDBASE_XX_H


#include "common_thing.h"


struct AttrInfo {
    char *attrName;
    AttrType attrType;
    int attrLength;
};

struct Value {
    AttrType type;
    void* data;
};


struct RelAttr {
    char* relname;
    char* attrname;
};

struct Condition {
    struct RelAttr  lhsAttr;    /* left-hand side attribute            */
    CompOp op;         /* comparison operator                 */
    bool      bRhsIsAttr;/* TRUE if the rhs is an attribute,    */
    /* in which case rhsAttr below is valid;*/
    /* otherwise, rhsValue below is valid.  */
    struct RelAttr  rhsAttr;    /* right-hand side attribute            */
    struct Value    rhsValue;   /* right-hand side value                */
    /* print function                       */

};
#endif //REDBASE_XX_H
