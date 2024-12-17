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

    IN,
    OUT,

    LBRACKET,
    RBRACKET,

    IF,

    BEGIN,
    ENDING,

    ASSIGN,
    SEP,

    NO_OP
};

typedef struct {
    const char * name;
    enum oper num;

    bool binary;
    bool commutative;

    const char * asm_str;
} oper_t;

const oper_t opers[] = {
    {.name = "+"  , .num = ADD, .binary = true,  .commutative = true , .asm_str = "ADD"},
    {.name = "-"  , .num = SUB, .binary = true,  .commutative = false, .asm_str = "SUB"},
    {.name = "*"  , .num = MUL, .binary = true,  .commutative = true , .asm_str = "MUL"},
    {.name = "/"  , .num = DIV, .binary = true,  .commutative = false, .asm_str = "DIV"},
    {.name = "^"  , .num = POW, .binary = true,  .commutative = false, .asm_str = "POW"},
    {.name = "sin", .num = SIN, .binary = false, .commutative = false, .asm_str = "SIN"},
    {.name = "cos", .num = COS, .binary = false, .commutative = false, .asm_str = "COS"},
    {.name = "tan", .num = TAN, .binary = false, .commutative = false, .asm_str = "TAN"},
    {.name = "ln" , .num = LN , .binary = false, .commutative = false, .asm_str = "LN" },
    {.name = "log", .num = LOG, .binary = true,  .commutative = false, .asm_str = NULL},
    {.name = "!"  , .num = FAC, .binary = false, .commutative = false, .asm_str = NULL},

    {.name = "in",  .num = IN , .binary = false, .commutative = false, .asm_str = NULL},
    {.name = "out", .num = OUT, .binary = false, .commutative = false, .asm_str = NULL},

    {.name = "("  , .num = LBRACKET, .binary = false, .commutative = false, .asm_str = NULL},
    {.name = ")"  , .num = RBRACKET, .binary = false, .commutative = false, .asm_str = NULL},

    {.name = "if",  .num = IF,  .binary = true, .commutative = false, .asm_str = NULL},

    {.name = "begin", .num = BEGIN,  .binary = false, .commutative = false, .asm_str = NULL},
    {.name = "end"  , .num = ENDING, .binary = false, .commutative = false, .asm_str = NULL},

    {.name = "="  , .num = ASSIGN  , .binary = true , .commutative = false, .asm_str = NULL},
    {.name = ";"  , .num = SEP     , .binary = true , .commutative = false, .asm_str = NULL},
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

const size_t NAME_MAX_LENGTH = 64;

typedef struct {
    char name[NAME_MAX_LENGTH];
    double value;
} idr_t;

typedef struct {
    node_t * cur_node;

    idr_t * ids;
    unsigned int id_size;
} tree_context_t;

void printTreePrefix(tree_context_t * tr, node_t * node);

void treeDumpGraph(tree_context_t * tree, node_t * root_node, const char * log_folder);

void treeMakeDot(tree_context_t * tr, node_t * node, FILE * dot_file);

#endif
