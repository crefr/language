#ifndef IR_HANDLER_INCLUDED
#define IR_HANDLER_INCLUDED

#include "tree.h"

const size_t NAME_MAX_LENGTH = 64;

typedef struct {
    char name[NAME_MAX_LENGTH];
    double value;
} idr_t;

typedef struct {
    node_t * cur_node;

    idr_t * ids;
    unsigned int id_size;
} IR_context;

char * readProgramText(const char * file_name);

void writeTreeToFile(IR_context * ir, node_t * root, FILE * out_file);

node_t * readTreeFromIR(IR_context * ir, const char * file_name);

#endif
