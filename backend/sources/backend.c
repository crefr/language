#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "logger.h"
#include "backend.h"
#include "IR_handler.h"

static void readTreeForBackend(be_context_t * be, const char * tree_file_name);

be_context_t backendInit(size_t nodes_num, const char * asm_file_name, const char * tree_file_name)
{
    assert(asm_file_name);
    assert(tree_file_name);

    be_context_t context = {};

    context.asm_file = fopen(asm_file_name, "w");
    assert(context.asm_file);   // TODO: make if-statement

    context.nodes = (node_t *)calloc(nodes_num, sizeof(node_t));
    context.tree_size = nodes_num;

    context.cur_node = context.nodes;
    context.id_size = 0;

    readTreeForBackend(&context, tree_file_name);

    return context;
}

static void readTreeForBackend(be_context_t * be, const char * tree_file_name)
{
    tree_context_t sub_context = {};

    sub_context.cur_node = be->cur_node;
    sub_context.id_size  = be->id_size;
    sub_context.ids      = be->ids;

    readTreeFromIR(&sub_context, tree_file_name);

    be->cur_node = sub_context.cur_node;
    be->id_size  = sub_context.id_size;
}

void backendDtor(be_context_t * context)
{
    assert(context);

    free(context->nodes);

    context->nodes    = NULL;
    context->cur_node = NULL;

    fclose(context->asm_file);
}

// void makeAssemblyCode(be_context_t * be)
// {
//     assert(be);
//
//
//
// }
