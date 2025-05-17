#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "backend_x64.h"
#include "logger.h"
#include "IR_handler.h"

const size_t MAX_NODES_NUM = 1024;


backend_ctx_t backendInit(const char * ast_file_name)
{
    assert(ast_file_name);

    backend_ctx_t ctx = {};
    ctx.root = (node_t *)calloc(MAX_NODES_NUM, sizeof(node_t));

    tree_context_t tree = {};
    tree.cur_node = ctx.root;

    ctx.root = readTreeFromIR(&tree, ast_file_name);
    ctx.id_table_size = tree.id_size;
    ctx.id_table      = tree.ids;

    return ctx;
}


void backendDestroy(backend_ctx_t * ctx)
{
    assert(ctx);

    free(ctx->id_table);
    free(ctx->root);

    ctx->id_table = NULL;
    ctx->root     = NULL;
}



