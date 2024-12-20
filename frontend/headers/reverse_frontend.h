#ifndef REVERSE_FRONTEND_INCLUDED
#define REVERSE_FRONTEND_INCLUDED

#include "tree.h"

void printCodeFromTree(const char * out_file_name, tree_context_t * context, node_t * root);

void reverseFrontendRun(const char * in_file_name, const char * out_file_name);

#endif
