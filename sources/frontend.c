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

    frontend.oper_table = tableCtor(OPR_TABLE_SIZE);
    for (size_t oper_index = 0; oper_index < opers_size; oper_index++){
        tableInsert(&(frontend.oper_table), opers[oper_index].name, &(opers[oper_index].num), sizeof(opers[oper_index].num));
    }

    frontend.var_table  = tableCtor(VAR_TABLE_SIZE);

    return frontend;
}

void frontendDtor(fe_context_t * frontend)
{
    assert(frontend);

    tableDtor(&(frontend->oper_table));
    tableDtor(&(frontend->var_table));

    free(frontend->tokens);
    frontend->tokens = NULL;
}

int lexicalAnalysis(fe_context_t * frontend, const char * code)
{
    assert(frontend);

    size_t token_index = 0;
    const char * cur_ch = code;

    while (*cur_ch != '\0'){
        if (isdigit(*cur_ch)){

            char * end = NULL;
            double number = strtod(cur_ch, &end);

            cur_ch = end;

            frontend->tokens[token_index].type = NUM;
            frontend->tokens[token_index].val.number = number;

            logPrint(LOG_DEBUG_PLUS, "LEXIC: scanned number: %lg\n", number);
        }

        else {
            char buffer[ID_MAX_LEN] = {};
            size_t buf_index = 0;

            if (isalpha(*cur_ch) || *cur_ch == '_'){
                while(isalpha(*cur_ch) || *cur_ch == '_' || isdigit(*cur_ch))
                    buffer[buf_index++] = *(cur_ch++);
            }
            else {
                while (!isalpha(*cur_ch) && !isdigit(*cur_ch) && !isspace(*cur_ch) && *cur_ch != '_')
                    buffer[buf_index++] = *(cur_ch++);
            }
            logPrint(LOG_DEBUG_PLUS, "LEXIC: scanned identificator: %s\n", buffer);

            name_t * identifier = NULL;

            // can be operator
            if ((identifier = tableLookup(&(frontend->oper_table), buffer)) != NULL){
                logPrint(LOG_DEBUG_PLUS, "\tis an operator\n");

                enum oper op_num = *((enum oper *)(identifier->data));

                frontend->tokens[token_index].type = OPR;
                frontend->tokens[token_index].val.op = op_num;
            }

            // so it is a variable
            else {
                logPrint(LOG_DEBUG_PLUS, "\tis a variable\n");

                unsigned int var_index = 0;

                if ((identifier = tableLookup(&(frontend->var_table), buffer)) == NULL){
                    logPrint(LOG_DEBUG_PLUS, "\t\tcreating new\n");

                    var_index = frontend->var_size;

                    strncpy(frontend->vars[var_index].name, buffer, buf_index);
                    frontend->var_size++;
                }
                else {
                    logPrint(LOG_DEBUG_PLUS, "\t\talready exists\n");

                    var_index = *((unsigned int *)(identifier->data));
                }

                frontend->tokens[token_index].type = VAR;
                frontend->tokens[token_index].val.var = var_index;
            }
        }

        token_index++;
    }

    return 0;
}
