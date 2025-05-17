#ifndef BACKEND_INCLUDED
#define BACKEND_INCLUDED

#include <stdint.h>

#include "tree.h"

const size_t MAX_IDR_NUM = 64;

// these constants MUST be negative, so they will not collide with real name_index
enum scope_start {
    START_OF_FUNC_SCOPE = -1,
    START_OF_SCOPE = -2
};

typedef struct {
    int64_t name_index; // index in the nametable or a number from enum scope_start
    size_t  address;
} idr_stack_elem_t;

// TODO: make it reallocating
typedef struct {
    idr_stack_elem_t elems[MAX_IDR_NUM];
    size_t size;
    size_t capacity;
} idr_stack_t;

typedef struct {
    FILE * asm_file;

    node_t * nodes;
    size_t tree_size;

    node_t * root;
    node_t * cur_node;

    idr_t * ids;
    unsigned int id_size;

    size_t if_counter;
    size_t while_counter;

    size_t global_var_counter;
    size_t  local_var_counter;

    bool in_function;

    idr_stack_t * idr_stack;
} be_context_t;

/// @brief initialise context structure and read the tree for further actions
be_context_t backendInit(size_t nodes_num, const char * asm_file_name, const char * tree_file_name);

/// @brief destruct context structure
void backendDtor(be_context_t * context);

/// @brief make assembly code and write it to file
void makeAssemblyCode(be_context_t * be);

#endif
