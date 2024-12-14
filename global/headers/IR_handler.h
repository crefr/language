#ifndef IR_HANDLER_INCLUDED
#define IR_HANDLER_INCLUDED

#include "tree.h"

char * readProgramText(const char * file_name);

void writeTreeToFile(tree_context_t * ir, node_t * root, FILE * out_file);

node_t * readTreeFromIR(tree_context_t * ir, const char * file_name);

#endif
