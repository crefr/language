#ifndef MIDDLEEND_INCLUDED
#define MIDDLEEND_INCLUDED

#include "tree.h"

const size_t MAX_IDR_NUM = 128;
const size_t MAX_NODES_NUM = 1024;

typedef struct {
    node_t * nodes;
    node_t * free_node;

    node_t * root;

    idr_t * ids;
    unsigned int id_size;
} me_context_t;

me_context_t middleendInit(const char * tree_file_name);

void middleendDestroy(me_context_t * me);

void middleendRun(const char * tree_file_name);

node_t * newNode(me_context_t * context, enum elem_type type, union value val, node_t * left, node_t * right);

node_t * newOprNode(me_context_t * context, enum oper op_num, node_t * left, node_t * right);

node_t * newNumNode(me_context_t * context, double number);

node_t * foldConstants(me_context_t * me, node_t * node, bool * changed_tree);

node_t * deleteNeutral(me_context_t * me, node_t * node, bool * changed_tree);

node_t * simplifyExpression(me_context_t * me, node_t * node);

#endif
