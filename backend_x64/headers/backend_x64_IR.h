#ifndef BACKEND_IR_INCLUDED
#define BACKEND_IR_INCLUDED

enum IR_type {
    IR_ADD        = 0,
    IR_SUB        = 1,
    IR_MUL        = 2,
    IR_DIV        = 3,

    IR_SET_FR_PTR = 4,
    IR_CALL       = 5,
    IR_RET        = 6,

    IR_JMP        = 7,
    IR_JZ         = 8,

    IR_COND_CHECK = 9,
};

typedef struct {
    enum IR_type type;
} IR_block_t;

typedef struct {
    node_t * root;

    idr_t * id_table;
    size_t id_table_size;

    IR_block_t * blocks;
    size_t capacity;
    size_t size;
} IR_context_t;


#endif
