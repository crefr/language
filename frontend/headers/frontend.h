#ifndef FRONTEND_INCLUDED
#define FRONTEND_INCLUDED

#include <stdbool.h>

#include "hashtable.h"
#include "tree.h"
#include "IR_handler.h"

const size_t MAX_CODE_LEN   = 1024;
const size_t MAX_TOKEN_NUM  = 1024;

const char * const LOG_FOLDER_NAME = "frontend_logs";
const char * const LOG_FILE_NAME   = "frontend_logs/log.html";

const char COMMENT_START = '#';
const char COMMENT_END   = '#';

// const size_t MAX_TOKEN_NUM = 1024;
const size_t MAX_IDR_NUM = 64;
const size_t NAME_MAX_LEN = 64;

const size_t IDR_TABLE_SIZE = 128;
const size_t OPR_TABLE_SIZE = 128;

const size_t ID_MAX_LEN = 64;

typedef enum {
    SUCCESS = 0,
    SOFT_ERROR,
    HARD_ERROR
} parser_status_t;

typedef struct {
    node_t * tokens;
    size_t tokens_size;

    node_t * cur_node;

    table_t oper_table;
    table_t  idr_table;

    idr_t ids[MAX_IDR_NUM];
    unsigned int id_size;

    parser_status_t status;
} fe_context_t;

/// @brief initialise frontend context
fe_context_t frontendInit(size_t token_num);

/// @brief destruct frontend context
void frontendDtor(fe_context_t * frontend);

/// @brief main function for frontend
void frontendRun(const char * in_file_name, const char * out_file_name);

/// @brief dump frontend info
void frontendDump(fe_context_t * frontend);

/// @brief do lexical analysis of the source code, returns 0 if succeeded
int lexicalAnalysis(fe_context_t * frontend, const char * code);

/// @brief start syntax analysis
node_t * parseCode(fe_context_t * frontend);


#endif
