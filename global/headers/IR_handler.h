#ifndef IR_HANDLER_INCLUDED
#define IR_HANDLER_INCLUDED

#include "tree.h"

struct oper_IR_name {
    enum oper op_num;
    const char * name;
};

const struct oper_IR_name oper_names[] = {
    {ADD,           "ADD"},
    {SUB,           "SUB"},
    {MUL,           "MUL"},
    {DIV,           "DIV"},

    {POW,           "POW"},
    {SQRT,          "SQRT"},

    {SIN,           "SIN"},
    {COS,           "COS"},
    {TAN,           "TAN"},

    {GREATER,       "GREATER"},
    {LESS,          "LESS"},
    {GREATER_EQ,    "GREATER_EQ"},
    {LESS_EQ,       "LESS_EQ"},
    {EQUAL,         "EQUAL"},
    {N_EQUAL,       "N_EQUAL"},

    {IN,            "IN"},
    {OUT,           "OUT"},

    {ASSIGN,        "ASSIGN"},

    {IF,            "IF"},
    {IF_ELSE,       "ELSE"},

    {WHILE,         "WHILE"},

    {SEP,           "SEP"},
    {ARG_SEP,       "ARG_SEP"},

    {VAR_DECL,      "VAR"},
    {FUNC_DECL,     "DEF"},

    {CALL,          "CALL"},
    {RETURN,        "RET"},

    {FUNC_HEADER,   "FUNC_HDR"},

    {TEXT, "TEXT"},

    //! must be the last !!!
    {NO_OP, "__UNKNOWN__"}
};
const size_t oper_names_num = sizeof(oper_names) / sizeof(oper_names[0]);
const size_t MAX_IR_OPER_NAME_LEN = 64;

char * readProgramText(const char * file_name);

void writeTreeToFile(tree_context_t * ir, node_t * root, FILE * out_file);

node_t * readTreeFromIR(tree_context_t * ir, const char * file_name);

#endif
