#ifndef BACKEND_X64_INCLUDED
#define BACKEND_X64_INCLUDED

#include "tree.h"


/* x64 backend
Global variables are on the stack (rbx is the start pointer


*/

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

typedef struct {
    node_t * root;

    FILE * asm_file;

    idr_t * id_table;
    size_t id_table_size;

    bool in_function;
    name_stack_t name_stack;
    size_t global_var_counter;
    size_t local_var_counter;

    size_t if_counter;
    size_t while_counter;
} backend_ctx_t;


backend_ctx_t backendInit(const char * ast_file_name);

void backendDestroy(backend_ctx_t * ctx);

void makeAssemblyCode(backend_ctx_t * ctx, const char * asm_file_name, const char * std_lib_file_name);
#endif
