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

typedef struct {
    const char * name;
    enum oper num;

    bool binary;
    bool commutative;
} oper_t;

const oper_t opers[] = {
    {.name = "+"  , .num = ADD, .binary = true,  .commutative = true },
    {.name = "-"  , .num = SUB, .binary = true,  .commutative = false},
    {.name = "*"  , .num = MUL, .binary = true,  .commutative = true },
    {.name = "/"  , .num = DIV, .binary = true,  .commutative = false},
    {.name = "^"  , .num = POW, .binary = true,  .commutative = false},
    {.name = "sin", .num = SIN, .binary = false, .commutative = false},
    {.name = "cos", .num = COS, .binary = false, .commutative = false},
    {.name = "tan", .num = TAN, .binary = false, .commutative = false},
    {.name = "ln" , .num = LN , .binary = false, .commutative = false},
    {.name = "log", .num = LOG, .binary = true,  .commutative = false},
    {.name = "!"  , .num = FAC, .binary = false, .commutative = false},

    {.name = "("  , .num = LBRACKET, .binary = false, .commutative = false},
    {.name = ")"  , .num = RBRACKET, .binary = false, .commutative = false},
    {.name = "="  , .num = ASSIGN  , .binary = true , .commutative = false},
    {.name = ";"  , .num = SEP     , .binary = true , .commutative = false},
};
const size_t opers_size = sizeof(opers) / sizeof(*opers);

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
