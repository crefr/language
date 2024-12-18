#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "frontend.h"
#include "logger.h"

fe_context_t frontendInit(size_t token_num)
{
    fe_context_t frontend = {};

    frontend.tokens = (node_t *)calloc(token_num, sizeof(node_t));
    frontend.cur_node = frontend.tokens;


    frontend.oper_table = tableCtor(OPR_TABLE_SIZE);
    for (size_t oper_index = 0; oper_index < opers_size; oper_index++){
        // name can be NULL when we actually do not want this operator in program text
        if (opers[oper_index].name != NULL)
            tableInsert(&(frontend.oper_table), opers[oper_index].name, &(opers[oper_index].num), sizeof(opers[oper_index].num));
    }

    frontend.idr_table  = tableCtor(IDR_TABLE_SIZE);

    return frontend;
}

void frontendDtor(fe_context_t * frontend)
{
    assert(frontend);

    tableDtor(&(frontend->oper_table));
    tableDtor(&(frontend->idr_table));

    free(frontend->tokens);
    frontend->tokens = NULL;
}

void frontendDump(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG, "------------frontend context dump-----------\n");

    logPrint(LOG_DEBUG, "\ttokens size: %zu\n", frontend->tokens_size);
    logPrint(LOG_DEBUG, "\ttokens:\n");

    for (size_t token_index = 0; token_index < frontend->tokens_size; token_index++){
        logPrint(LOG_DEBUG, "\t\ttoken #%zu:\n", token_index);
        node_t cur_token = frontend->tokens[token_index];

        if (cur_token.type == NUM){
            logPrint(LOG_DEBUG, "\t\t\ttype = NUM\n\t\t\tvalue = %lg\n", cur_token.val.number);
        }
        else if (cur_token.type == IDR){
            logPrint(LOG_DEBUG, "\t\t\ttype = IDR\n\t\t\tvalue = '%s'\n", frontend->ids[cur_token.val.id].name);
        }
        else if (cur_token.type == OPR){
            logPrint(LOG_DEBUG, "\t\t\ttype = OPR\n\t\t\tvalue = '%s'\n", opers[cur_token.val.op].name);
        }
        else {
            logPrint(LOG_DEBUG, "\t\t\ttype = END\n");
        }
    }

    logPrint(LOG_DEBUG, "\tids size: %zu\n", frontend->id_size);
    logPrint(LOG_DEBUG, "\tids:\n");

    for (size_t id_index = 0; id_index < frontend->id_size; id_index++){
        logPrint(LOG_DEBUG, "\t\tid #%zu:\n", id_index);
        idr_t cur_id = frontend->ids[id_index];

        logPrint(LOG_DEBUG, "\t\t\tname = '%s'\n\t\t\ttype = %s\n", cur_id.name, (cur_id.type == VAR) ? "VAR" : "FUNC");
    }

    logPrint(LOG_DEBUG, "----------frontend context dump end---------\n");
}
