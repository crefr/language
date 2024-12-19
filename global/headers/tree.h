#ifndef TREE_INCLUDED
#define TREE_INCLUDED

#include <stdbool.h>
#include <stdlib.h>

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

    GREATER,
    LESS,
    GREATER_EQ,
    LESS_EQ,
    EQUAL,
    N_EQUAL,

    IN,
    OUT,

    LBRACKET,
    RBRACKET,

    IF,
    IF_ELSE,

    WHILE,

    VAR_DECL,

    CALL,
    FUNC_DECL,
    FUNC_HEADER,
    ARG_SEP,
    RETURN,

    BEGIN,
    ENDING,

    ASSIGN,
    SEP,

    NO_OP
};

typedef struct {
    enum oper num;
    const char * name;
    const char * dot_name;

    bool binary;
    bool commutative;

    const char * asm_str;
} oper_t;

const oper_t opers[] = {
    {.num = ADD, .name = "+"  , .dot_name = "+",   .binary = true,  .commutative = true , .asm_str = "ADD"},
    {.num = SUB, .name = "-"  , .dot_name = "-",   .binary = true,  .commutative = false, .asm_str = "SUB"},
    {.num = MUL, .name = "*"  , .dot_name = "*",   .binary = true,  .commutative = true , .asm_str = "MUL"},
    {.num = POW, .name = "^"  , .dot_name = "^",   .binary = true,  .commutative = false, .asm_str = "POW"},
    {.num = DIV, .name = "/"  , .dot_name = "/",   .binary = true,  .commutative = false, .asm_str = "DIV"},
    {.num = SIN, .name = "sin", .dot_name = "sin", .binary = false, .commutative = false, .asm_str = "SIN"},
    {.num = COS, .name = "cos", .dot_name = "cos", .binary = false, .commutative = false, .asm_str = "COS"},
    {.num = TAN, .name = "tan", .dot_name = "tan", .binary = false, .commutative = false, .asm_str = "TAN"},
    {.num = LN , .name = "ln" , .dot_name = "ln",  .binary = false, .commutative = false, .asm_str = "LN" },
    {.num = LOG, .name = "log", .dot_name = "log", .binary = true,  .commutative = false, .asm_str = NULL},
    {.num = FAC, .name = "!"  , .dot_name = "!",   .binary = false, .commutative = false, .asm_str = NULL},

    {.num = GREATER,    .name = ">"  , .dot_name = "GREATER",    .binary = true, .commutative = false, .asm_str = "CALL __GREATER_OP__:"},
    {.num = LESS   ,    .name = "<"  , .dot_name = "LESS",       .binary = true, .commutative = false, .asm_str = "CALL __LESS_OP__:"},
    {.num = GREATER_EQ, .name = ">=" , .dot_name = "GREATER_EQ", .binary = true, .commutative = false, .asm_str = "CALL __GREATER_EQ_OP__:"},
    {.num = LESS_EQ   , .name = "<=" , .dot_name = "LESS_EQ",    .binary = true, .commutative = false, .asm_str = "CALL __LESS_EQ_OP__:"},
    {.num = EQUAL   ,   .name = "==" , .dot_name = "EQUAL",      .binary = true, .commutative = false, .asm_str = "CALL __EQUAL_OP__:"},
    {.num = N_EQUAL   , .name = "!=" , .dot_name = "N_EQUAL",    .binary = true, .commutative = false, .asm_str = "CALL __N_EQUAL_OP__:"},

    {.num = IN , .name = "in" , .dot_name = "in" , .binary = false, .commutative = false, .asm_str = NULL},
    {.num = OUT, .name = "out", .dot_name = "out", .binary = false, .commutative = false, .asm_str = NULL},

    {.num = LBRACKET, .name = "(", .dot_name = "(", .binary = false, .commutative = false, .asm_str = NULL},
    {.num = RBRACKET, .name = ")", .dot_name = ")", .binary = false, .commutative = false, .asm_str = NULL},

    {.num = IF,      .name = "if"   , .dot_name = "if",   .binary = true, .commutative = false, .asm_str = NULL},
    {.num = IF_ELSE, .name = "else" , .dot_name = "else", .binary = true, .commutative = false, .asm_str = NULL},

    {.num = WHILE, .name = "while", .dot_name = "while", .binary = true, .commutative = false, .asm_str = NULL},

    {.num = VAR_DECL, .name = "var", .dot_name = "var", .binary = false, .commutative = false, .asm_str = NULL},

    {.num = CALL,        .name = NULL,     .dot_name = "CALL",        .binary = true,  .commutative = false, .asm_str = NULL},
    {.num = FUNC_DECL,   .name = "func",   .dot_name = "func",        .binary = true,  .commutative = false, .asm_str = NULL},
    {.num = FUNC_HEADER, .name = NULL,     .dot_name = "FUNC_HEADER", .binary = true,  .commutative = false, .asm_str = NULL},
    {.num = ARG_SEP,     .name = ",",      .dot_name = "ARG_SEP",     .binary = true,  .commutative = false, .asm_str = NULL},
    {.num = RETURN,      .name = "return", .dot_name = "RETURN",      .binary = false, .commutative = false, .asm_str = NULL},

    { .num = BEGIN,  .name = "begin", .dot_name = "begin", .binary = false, .commutative = false, .asm_str = NULL},
    { .num = ENDING, .name = "end"  , .dot_name = "end"  , .binary = false, .commutative = false, .asm_str = NULL},

    {.num = ASSIGN  , .name = "="  , .dot_name = "="  , .binary = true , .commutative = false, .asm_str = NULL},
    {.num = SEP     , .name = ";"  , .dot_name = "SEP", .binary = true , .commutative = false, .asm_str = NULL},
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

enum id_type {
    VAR,
    FUNC
};

typedef struct {
    char name[NAME_MAX_LENGTH];
    enum id_type type;         // is VAR by default

    bool is_local;
    size_t parent_function;    // for local vars to show which function they are belonging to

    long int address;          // address of the var in RAM, if local, must be added to base pointer
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
