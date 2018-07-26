
#ifndef REDBASE_COMP_H
#define REDBASE_COMP_H

#include "common_thing.h"

class Comp {
public:
    Comp(AttrType type, int len) : type_(type), len_(len) {}
    virtual ~Comp() {}
public:
    virtual bool eval(const void* lhs, CompOp op, const void* rhs) = 0;
protected:
    AttrType type_;  /* 类型 */
    int len_;		 /* 长度 */
};

Comp* make_comp(AttrType type, int len);


#endif //REDBASE_COMP_H
