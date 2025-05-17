#ifndef BACKEND_X64_INCLUDED
#define BACKEND_X64_INCLUDED

#include "tree.h"

typedef struct {
    node_t * root;
    idr_t * id_table;
    size_t id_table_size;
} backend_ctx_t;


backend_ctx_t backendInit(const char * ast_file_name);

void backendDestroy(backend_ctx_t * ctx);

#endif
