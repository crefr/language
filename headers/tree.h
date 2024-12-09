#ifndef TREE_INCLUDED
#define TREE_INCLUDED

enum elem_type{
    NUM = 0,
    OPR = 1,
    VAR = 2
};

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
    FAC
};

union value {
    double number;
    unsigned int var;
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
