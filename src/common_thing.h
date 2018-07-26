#ifndef REDBASE_COMMON_THING_H
#define REDBASE_COMMON_THING_H
#define RC int



#define OK_RC			0				// OK_RC return code is guaranteed to always be 0

#define START_PF_ERR  (-1)
#define END_PF_ERR    (-100)
#define START_RM_ERR  (-101)
#define END_RM_ERR    (-200)
#define START_IX_ERR  (-201)
#define END_IX_ERR    (-300)
#define START_SM_ERR  (-301)
#define END_SM_ERR    (-400)
#define START_QL_ERR  (-401)
#define END_QL_ERR    (-500)

#define START_PF_WARN  1
#define END_PF_WARN    100
#define START_RM_WARN  101
#define END_RM_WARN    200
#define START_IX_WARN  201
#define END_IX_WARN    300
#define START_SM_WARN  301
#define END_SM_WARN    400
#define START_QL_WARN  401
#define END_QL_WARN    500


enum AttrType {
    INT,
    FLOAT,
    STRING
};

//
// Comparison operators
//
enum CompOp {
    NO_OP, //没有比较
    EQ_OP, //相等
    NE_OP, //不等
    LT_OP, //小于
    GT_OP, //大于
    LE_OP, //小于等于
    GE_OP  //大于等于
};

enum ClientHint {
    NO_HINT                                     // default value
};

#define MAXNAME       24                // maximum length of a relation or attribute name

#define MAXSTRINGLEN  255               // maximum length of a string-type attribute

#define MAXATTRS      40                // maximum number of attributes in a relation

#endif //REDBASE_COMMON_THING_H
