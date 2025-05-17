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
    ADD      = 0,
    SUB      = 1,
    MUL      = 2,
    DIV      = 3,
    POW      = 4,
    SQRT     = 5,
    SIN      = 6,
    COS      = 7,
    TAN      = 8,
    LN       = 9,
    LOG      = 10,
    FAC      = 11,

    GREATER  = 12,
    LESS     = 13,
    GREATER_EQ = 14,
    LESS_EQ  = 15,
    EQUAL    = 16,
    N_EQUAL  = 17,

    IN       = 18,
    OUT      = 19,

    LBRACKET = 20,
    RBRACKET = 21,

    IF       = 22,
    IF_ELSE  = 23,

    WHILE    = 24,

    VAR_DECL = 25,

    CALL     = 26,
    FUNC_DECL = 27,
    FUNC_HEADER = 28,
    ARG_SEP  = 29,
    RETURN   = 30,

    BEGIN    = 31,
    ENDING   = 32,

    ASSIGN   = 33,
    SEP      = 34,

    NO_OP    = 35
};

typedef struct {
    enum oper num;

    const char * name;
    const char * dot_name;

    bool binary;
    bool commutative;

    const char * asm_str;

    bool can_simple;
} oper_t;

const oper_t opers[] = {
    {.num = ADD, .name = "+"   , .dot_name = "+",   .binary = true,  .commutative = true , .asm_str = "ADD", .can_simple = true},
    {.num = SUB, .name = "-"   , .dot_name = "-",   .binary = true,  .commutative = false, .asm_str = "SUB", .can_simple = true},
    {.num = MUL, .name = "*"   , .dot_name = "*",   .binary = true,  .commutative = true , .asm_str = "MUL", .can_simple = true},
    {.num = DIV, .name = "/"   , .dot_name = "/",   .binary = true,  .commutative = false, .asm_str = "DIV", .can_simple = true},
    {.num = POW, .name = "^"   , .dot_name = "^",   .binary = true,  .commutative = false, .asm_str = "POW", .can_simple = true},
    {.num = SQRT,.name = "sqrt", .dot_name = "sqrt",.binary = false, .commutative = false, .asm_str = "SQRT",.can_simple = false},
    {.num = SIN, .name = "sin" , .dot_name = "sin", .binary = false, .commutative = false, .asm_str = "SIN", .can_simple = true},
    {.num = COS, .name = "cos" , .dot_name = "cos", .binary = false, .commutative = false, .asm_str = "COS", .can_simple = true},
    {.num = TAN, .name = "tan" , .dot_name = "tan", .binary = false, .commutative = false, .asm_str = "TAN", .can_simple = true},
    {.num = LN , .name = "ln"  , .dot_name = "ln",  .binary = false, .commutative = false, .asm_str = "LN" , .can_simple = true},
    {.num = LOG, .name = "log" , .dot_name = "log", .binary = true,  .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = FAC, .name = "!"   , .dot_name = "!",   .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = GREATER,    .name = ">"  , .dot_name = "GREATER",    .binary = true, .commutative = false, .asm_str = "CALL __GREATER_OP__:", .can_simple = false},
    {.num = LESS   ,    .name = "<"  , .dot_name = "LESS",       .binary = true, .commutative = false, .asm_str = "CALL __LESS_OP__:", .can_simple = false},
    {.num = GREATER_EQ, .name = ">=" , .dot_name = "GREATER_EQ", .binary = true, .commutative = false, .asm_str = "CALL __GREATER_EQ_OP__:", .can_simple = false},
    {.num = LESS_EQ   , .name = "<=" , .dot_name = "LESS_EQ",    .binary = true, .commutative = false, .asm_str = "CALL __LESS_EQ_OP__:", .can_simple = false},
    {.num = EQUAL   ,   .name = "==" , .dot_name = "EQUAL",      .binary = true, .commutative = false, .asm_str = "CALL __EQUAL_OP__:", .can_simple = false},
    {.num = N_EQUAL   , .name = "!=" , .dot_name = "N_EQUAL",    .binary = true, .commutative = false, .asm_str = "CALL __N_EQUAL_OP__:", .can_simple = false},

    {.num = IN , .name = "in" , .dot_name = "in" , .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = OUT, .name = "out", .dot_name = "out", .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = LBRACKET, .name = "(", .dot_name = "(", .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = RBRACKET, .name = ")", .dot_name = ")", .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = IF,      .name = "if"   , .dot_name = "if",   .binary = true, .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = IF_ELSE, .name = "else" , .dot_name = "else", .binary = true, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = WHILE, .name = "while", .dot_name = "while", .binary = true, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = VAR_DECL, .name = "var", .dot_name = "var", .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = CALL,        .name = NULL,     .dot_name = "CALL",        .binary = true,  .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = FUNC_DECL,   .name = "func",   .dot_name = "func",        .binary = true,  .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = FUNC_HEADER, .name = NULL,     .dot_name = "FUNC_HEADER", .binary = true,  .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = ARG_SEP,     .name = ",",      .dot_name = "ARG_SEP",     .binary = true,  .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = RETURN,      .name = "return", .dot_name = "RETURN",      .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},

    { .num = BEGIN,  .name = "begin", .dot_name = "begin", .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},
    { .num = ENDING, .name = "end"  , .dot_name = "end"  , .binary = false, .commutative = false, .asm_str = NULL, .can_simple = false},

    {.num = ASSIGN  , .name = "="  , .dot_name = "="  , .binary = true , .commutative = false, .asm_str = NULL, .can_simple = false},
    {.num = SEP     , .name = ";"  , .dot_name = "SEP", .binary = true , .commutative = false, .asm_str = NULL, .can_simple = false},
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

    size_t num_of_args;
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
