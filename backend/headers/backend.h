#ifndef BACKEND_INCLUDED
#define BACKEND_INCLUDED

#include "tree.h"

const size_t MAX_IDR_NUM = 64;

typedef struct {
    FILE * asm_file;

    node_t * nodes;
    size_t tree_size;

    node_t * root;
    node_t * cur_node;

    idr_t ids[MAX_IDR_NUM];
    unsigned int id_size;

    size_t if_counter;
    size_t while_counter;

    size_t global_var_counter;
    size_t  local_var_counter;

    bool in_function;
} be_context_t;

/// @brief initialise context structure and read the tree for further actions
be_context_t backendInit(size_t nodes_num, const char * asm_file_name, const char * tree_file_name);

/// @brief destruct context structure
void backendDtor(be_context_t * context);

/// @brief make assembly code and write it to file
void makeAssemblyCode(be_context_t * be);

#endif
