#ifndef BACKEND_X64_INCLUDED
#define BACKEND_X64_INCLUDED

#include <stdint.h>

#include "tree.h"
#include "x64_emitters.h"

// these constants MUST be negative, so they will not collide with real name_index
enum scope_start {
    START_OF_FUNC_SCOPE = -1,
    START_OF_SCOPE = -2
};

const size_t NAME_STACK_START_CAP = 16;

typedef struct {
    int64_t name_index; // index in the nametable or a number from enum scope_start
    int64_t  rel_addr;
    bool is_global;
} name_addr_t;

typedef struct {
    name_addr_t * elems;
    size_t size;
    size_t capacity;
} name_stack_t;


enum IR_type {
    IR_ADD        = 0,
    IR_SUB        = 1,
    IR_MUL        = 2,
    IR_DIV        = 3,

    IR_SET_FR_PTR = 4,
    IR_CALL       = 5,
    IR_RET        = 6,

    IR_COND_JMP   = 7,
    IR_JMP        = 9,

    IR_LABEL      = 10,

    IR_PUSH_IMM   = 11,
    IR_PUSH_MEM   = 12,
    IR_POP_MEM    = 13,

    IR_VAR_DECL   = 14,

    IR_OUT        = 15,
    IR_IN         = 16,

    IR_SQRT       = 17,

    IR_GREATER    = 18,
    IR_LESS       = 19,
    IR_GREATER_EQ = 20,
    IR_LESS_EQ    = 21,
    IR_EQUAL      = 22,
    IR_N_EQUAL    = 23,

    IR_START      = 24,
    IR_EXIT       = 25
};


const size_t MAX_LABEL_NAME_LEN = 64;

typedef struct {
    enum IR_type type;

    union {
        size_t label_block_idx;     //< for jumps and calls
        name_addr_t var;          //< for commands that work with vars
        int64_t imm_val;            //< for push_imm

        char label_name[MAX_LABEL_NAME_LEN];    //< for labels
    };

    size_t name_id;
    size_t arg_num;                 //< for funcs

    int32_t addr;
} IR_block_t;

const size_t IR_START_CAP = 1024;

typedef struct {
    IR_block_t * blocks;
    size_t capacity;
    size_t size;

    int32_t std_in_addr;
    int32_t std_out_addr;
} IR_context_t;

typedef struct {
    node_t * root;

    FILE * asm_file;
    emit_ctx_t * emit;

    idr_t * id_table;
    size_t id_table_size;

    bool in_function;
    name_stack_t name_stack;
    size_t global_var_counter;
    size_t local_var_counter;

    size_t if_counter;
    size_t while_counter;

    IR_context_t IR;
} backend_ctx_t;


backend_ctx_t backendInit(const char * ast_file_name);

void backendDestroy(backend_ctx_t * ctx);

void makeIR(backend_ctx_t * ctx);


#endif
