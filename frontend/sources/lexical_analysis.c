#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "logger.h"
#include "frontend.h"

enum id_stat {
    IS_IDR,
    IS_OPR,
    IS_IDR_OR_OPR
};

static enum id_stat getIdName(const char ** src_str, char * buffer);

static void skipSpaces(const char ** code);

int lexicalAnalysis(fe_context_t * frontend, const char * code)
{
    assert(frontend);
    assert(code);

    size_t token_index = 0;
    const char * cur_ch = code;

    skipSpaces(&cur_ch);

    while (*cur_ch != '\0'){
        if (isdigit(*cur_ch)){

            char * end = NULL;
            double number = strtod(cur_ch, &end);

            cur_ch = end;

            frontend->tokens[token_index].type = NUM;
            frontend->tokens[token_index].val.number = number;

            logPrint(LOG_DEBUG_PLUS, "LEXIC: scanned number: %lg\n", number);
        }

        else if (*cur_ch == '(' || *cur_ch == ')'){
            enum oper op_num = (*cur_ch == '(') ? LBRACKET : RBRACKET;
            cur_ch++;

            frontend->tokens[token_index].type = OPR;
            frontend->tokens[token_index].val.op = op_num;
        }

        else {
            char buffer[ID_MAX_LEN] = {};

            enum id_stat id_status = getIdName(&cur_ch, buffer);

            logPrint(LOG_DEBUG_PLUS, "LEXIC: scanned identificator: %s\n", buffer);

            name_t * identifier = NULL;

            // can be operator
            if ((id_status == IS_IDR_OR_OPR || id_status == IS_OPR) && (identifier = tableLookup(&(frontend->oper_table), buffer)) != NULL){
                logPrint(LOG_DEBUG_PLUS, "\tis an operator\n");

                enum oper op_num = *((enum oper *)(identifier->data));

                frontend->tokens[token_index].type = OPR;
                frontend->tokens[token_index].val.op = op_num;
            }

            // so it is an id
            else if (id_status == IS_IDR || id_status == IS_IDR_OR_OPR){
                logPrint(LOG_DEBUG_PLUS, "\tis a identifier\n");

                unsigned int id_index = 0;

                if ((identifier = tableLookup(&(frontend->idr_table), buffer)) == NULL){
                    logPrint(LOG_DEBUG_PLUS, "\t\tcreating new\n");

                    id_index = frontend->id_size;

                    strcpy(frontend->ids[id_index].name, buffer);
                    frontend->ids[id_index].type = VAR; // VAR is default value

                    frontend->id_size++;

                    tableInsert(&frontend->idr_table, buffer, &id_index, sizeof(id_index));
                }
                else {
                    logPrint(LOG_DEBUG_PLUS, "\t\talready exists\n");

                    id_index = *((unsigned int *)(identifier->data));
                }

                frontend->tokens[token_index].type = IDR;
                frontend->tokens[token_index].val.id = id_index;
            }
            else {
                logPrint(LOG_RELEASE, "LEXIC ERROR: unknown opeator '%s'\n", buffer);
                fprintf(stderr, "LEXIC ERROR: unknown operator '%s'\n", buffer);

                return 1;
            }
        }
        skipSpaces(&cur_ch);

        token_index++;
    }

    frontend->tokens[token_index++].type = END;

    frontend->tokens_size = token_index;

    return 0;
}

static enum id_stat getIdName(const char ** src_str, char * buffer)
{
    assert(src_str);
    assert(buffer);

    if (isalpha(**src_str) || **src_str == '_'){
        while((**src_str != '\0') && (**src_str != COMMENT_START) && (isalpha(**src_str) || **src_str == '_' || isdigit(**src_str)))
            *(buffer++) = *((*src_str)++);

        return IS_IDR_OR_OPR;
    }

    while ((**src_str != '\0') && (**src_str != COMMENT_START) && (!isalpha(**src_str) && !isdigit(**src_str) && !isspace(**src_str) && **src_str != '_'))
        *(buffer++) = *((*src_str)++);

    return IS_OPR;
}

static void skipSpaces(const char ** code)
{
    assert(code);
    assert(*code);

    bool in_comment = false;

    while ((**code != '\0') && (in_comment || isspace(**code) || **code == COMMENT_START)){
        if (in_comment && **code == COMMENT_END)
            in_comment = false;
        else if (**code == COMMENT_START)
            in_comment = true;


        (*code)++;
    }
}
