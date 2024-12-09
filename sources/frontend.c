#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "frontend.h"
#include "logger.h"

enum id_stat {
    IS_VAR,
    IS_OPR,
    IS_VAR_OR_OPR
};

static enum id_stat getIdName(const char ** src_str, char * buffer);

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

            enum id_stat id_status = getIdName(&cur_ch, buffer);

            logPrint(LOG_DEBUG_PLUS, "LEXIC: scanned identificator: %s\n", buffer);

            name_t * identifier = NULL;

            // can be operator
            if ((id_status == IS_VAR_OR_OPR || id_status == IS_OPR) && (identifier = tableLookup(&(frontend->oper_table), buffer)) != NULL){
                logPrint(LOG_DEBUG_PLUS, "\tis an operator\n");

                enum oper op_num = *((enum oper *)(identifier->data));

                frontend->tokens[token_index].type = OPR;
                frontend->tokens[token_index].val.op = op_num;
            }

            // so it is a variable
            else if (id_status == IS_VAR || id_status == IS_VAR_OR_OPR){
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
            else {
                logPrint(LOG_RELEASE, "LEXIC ERROR: unknown opeator %s\n", buffer);
                fprintf(stderr, "LEXIC ERROR: unknown operator %s\n", buffer);

                return 1;
            }
        }

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
        while((**src_str != '\0') && (isalpha(**src_str) || **src_str == '_' || isdigit(**src_str)))
            *(buffer++) = *((*src_str)++);

        return IS_VAR_OR_OPR;
    }

    while ((**src_str != '\0') && (!isalpha(**src_str) && !isdigit(**src_str) && !isspace(**src_str) && **src_str != '_'))
        *(buffer++) = *((*src_str)++);

    return IS_OPR;
}


/*----------------------recursive descent----------------------*/
static node_t * getExpr(fe_context_t * frontend);

static node_t * getMulDiv(fe_context_t * frontend);

static node_t * getPower(fe_context_t * frontend);

static node_t * getPrimary(fe_context_t * frontend);

static node_t * getNumber(fe_context_t * frontend);

static node_t * getFunc(fe_context_t * frontend);

static node_t * getVar(fe_context_t * frontend);

static void syntaxError(const char * expected, node_t * node);

#define token (frontend->cur_node)

node_t * parseCode(fe_context_t * frontend)
{
    assert(frontend);

    frontend->cur_node = frontend->tokens;

    node_t * root = getExpr(frontend);

    if (root == NULL){
        fprintf(stderr, "failed to parse\n");
        return NULL;
    }

    if (frontend->cur_node->type != END){
        fprintf(stderr, "failed to parse (no end)\n");
        return NULL;
    }

    return root;
}

static node_t * getExpr(fe_context_t * frontend)
{
    assert(frontend);

    node_t * node = getMulDiv(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (token->type == OPR && (token->val.op == ADD || token->val.op == SUB)){
        node_t * node2 = getMulDiv(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        token->left  = node;
        token->right = node2;

        node = token;

        token++;
    }

    return node;
}

static node_t * getMulDiv(fe_context_t * frontend)
{
    assert(frontend);

    node_t * node = getPower(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (token->type == OPR && (token->val.op == MUL || token->val.op == DIV)){
        node_t * node2 = getPower(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        token->left  = node;
        token->right = node2;

        node = token;

        token++;
    }

    return node;
}

static node_t * getPower(fe_context_t * frontend)
{
    assert(frontend);

    node_t * node = getPrimary(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (token->type == OPR && token->val.op == POW){
        node_t * node2 = getPower(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        token->left  = node;
        token->right = node2;

        node = token;

        token++;
    }

    return node;
}

static node_t * getPrimary(fe_context_t * frontend)
{
    assert(frontend);

    if (token->type == OPR && token->val.op == LBRACKET){
        token++;

        node_t * node = getExpr(frontend);

        if (!(token->type == OPR && token->val.op == RBRACKET)){
            frontend->status = HARD_ERROR;
            syntaxError(")", token);

            return NULL;
        }

        token++;

        return node;
    }

    node_t * func = getFunc(frontend);

    if (frontend->status == SUCCESS)
        return func;
    else if (frontend->status == HARD_ERROR)
        return NULL;

    node_t * var = getVar(frontend);

    if (frontend->status == SUCCESS)
        return var;
    else if (frontend->status == HARD_ERROR)
        return NULL;

    node_t * number = getNumber(frontend);
    if (frontend->status == SUCCESS)
        return number;

    return NULL;
}

static node_t * getNumber(fe_context_t * frontend)
{
    assert(frontend);

    if (token->type == NUM){
        node_t * num_node = token;
        token++;

        return num_node;
    }
    frontend->status = HARD_ERROR;

    return NULL;
}

static node_t * getVar(fe_context_t * frontend)
{
    assert(frontend);

    if (!(token->type == VAR)){
        frontend->status  = SOFT_ERROR;

        return NULL;
    }
    frontend->status = SUCCESS;

    node_t * var_node = token;
    token++;

    return var_node;
}

static node_t * getFunc(fe_context_t * frontend)
{
    assert(frontend);

    if (!(token->type == OPR)){
        frontend->status = SOFT_ERROR;

        return NULL;
    }

    switch (token->val.op){
        case SIN: case COS: case LN: case TAN:{
            node_t * func_node = token;
            token++;

            if (!(token->type == OPR && token->val.op == LBRACKET)){
                frontend->status = SOFT_ERROR;

                return NULL;
            }
            token++;

            func_node->left = getExpr(frontend);

            if (!(token->type == OPR && token->val.op)){
                frontend->status = HARD_ERROR;

                return NULL;
            }

            return func_node;
        }

        default:
            frontend->status = SOFT_ERROR;
            return NULL;
    }

    return NULL;
}

#undef token

static void syntaxError(const char * expected, node_t * node)
{
    assert(expected);
}
