#ifndef TREE_INCLUDED
#define TREE_INCLUDED

enum elem_type{
    NUM = 0,
    OPR = 1,
    IDR = 2,
    END = -1
};
const size_t MAX_ELEM_TYPE_NAME_LEN = 10;

enum oper{
    ADD = 0,
    SUB,
    MUL,
    DIV,
    POW,
    SIN,
    COS,
    TAN,
    LN,
    LOG,
    FAC,

    LBRACKET,
    RBRACKET,
    ASSIGN,
    SEP,

    NO_OP
};

union value {
    double number;
    unsigned int id;
    enum oper op;
};

typedef struct node {
    size_t line_num;
    size_t char_count;

    enum elem_type type;
    union value val;

    struct node * left;
    struct node * right;
} node_t;

#endif
