#ifndef FRONTEND_INCLUDED
#define FRONTEND_INCLUDED

#include "hashtable.h"
#include "tree.h"

// const size_t MAX_TOKEN_NUM = 1024;
const size_t MAX_VAR_NUM = 64;
const size_t NAME_MAX_LEN = 64;

const size_t VAR_TABLE_SIZE = 128;
const size_t OPR_TABLE_SIZE = 128;

const size_t ID_MAX_LEN = 64;

typedef struct {
    char name[NAME_MAX_LEN];
    double value;
} var_t;

typedef struct {
    const char * name;
    enum oper num;

    bool binary;
    bool commutative;
} oper_t;

typedef enum {
    SUCCESS = 0,
    SOFT_ERROR,
    HARD_ERROR
} parser_status_t;

typedef struct {
    node_t * tokens;
    size_t tokens_size;

    node_t * cur_node;

    table_t oper_table;
    table_t  var_table;

    var_t vars[MAX_VAR_NUM];
    unsigned int var_size;

    parser_status_t status;
} fe_context_t;

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
    {.name = ")"  , .num = RBRACKET, .binary = false, .commutative = false}
};
const size_t opers_size = sizeof(opers) / sizeof(*opers);

/// @brief initialise frontend context
fe_context_t frontendInit(size_t token_num);

/// @brief destruct frontend context
void frontendDtor(fe_context_t * frontend);

/// @brief do lexical analysis of the source code, returns 0 if succeeded
int lexicalAnalysis(fe_context_t * frontend, const char * code);

node_t * parseCode(fe_context_t * frontend);

#endif
