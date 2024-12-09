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

static void skipSpaces(const char ** code);

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
        else if (cur_token.type == VAR){
            logPrint(LOG_DEBUG, "\t\t\ttype = VAR\n\t\t\tvalue = '%s'\n", frontend->vars[cur_token.val.var].name);
        }
        else if (cur_token.type == OPR){
            logPrint(LOG_DEBUG, "\t\t\ttype = OPR\n\t\t\tvalue = '%s'\n", opers[cur_token.val.op]);
        }
        else {
            logPrint(LOG_DEBUG, "\t\t\ttype = END\n");
        }
    }

    logPrint(LOG_DEBUG, "\tvars size: %zu\n", frontend->tokens_size);
    logPrint(LOG_DEBUG, "\tvars:\n");

    for (size_t var_index = 0; var_index < frontend->var_size; var_index++){
        logPrint(LOG_DEBUG, "\t\tvar #%zu:\n", var_index);
        var_t cur_var = frontend->vars[var_index];

        logPrint(LOG_DEBUG, "\t\t\tname = '%s'\n\t\t\tvalue = %lg\n", cur_var.name, cur_var.value);
    }

    logPrint(LOG_DEBUG, "----------frontend context dump end---------\n");
}

int lexicalAnalysis(fe_context_t * frontend, const char * code)
{
    assert(frontend);

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

                    strcpy(frontend->vars[var_index].name, buffer);

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

        return IS_VAR_OR_OPR;
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


/*----------------------recursive descent----------------------*/

static node_t * getChain(fe_context_t * frontend);

static node_t * getAssign(fe_context_t * frontend);

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

    node_t * root = getChain(frontend);

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

static node_t * getChain(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getChain   (token #%zu)\n", (size_t)(token - frontend->tokens));

    node_t * node = NULL;
    node_t * last = NULL;

    while ((last = getAssign(frontend)) != NULL){
        if (!(token->type == OPR && token->val.op == SEP)){
            frontend->status = HARD_ERROR;

            return NULL;
        }

        token->left  = node;
        token->right = last;

        node = token;
        token++;
    }

    return node;
}

static node_t * getAssign(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getAssign  (token #%zu)\n", (size_t)(token - frontend->tokens));

    node_t * left_part = getVar(frontend);
    if (frontend->status != SUCCESS){
        return NULL;
    }

    node_t * assign_node = token;
    if (!(token->type == OPR && token->val.op == ASSIGN)){
        syntaxError("=", token);
        return NULL;
    }
    token++;

    node_t * right_part = getExpr(frontend);
    if (frontend->status != SUCCESS){
        return NULL;
    }

    assign_node->left  =  left_part;
    assign_node->right = right_part;

    return assign_node;
}

static node_t * getExpr(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getExpr    (token #%zu)\n", (size_t)(token - frontend->tokens));

    node_t * node = getMulDiv(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (token->type == OPR && (token->val.op == ADD || token->val.op == SUB)){
        node_t * cur_node = token;
        token++;

        node_t * node2 = getMulDiv(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        cur_node->left  = node;
        cur_node->right = node2;

        node = cur_node;
    }

    return node;
}

static node_t * getMulDiv(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getMulDiv  (token #%zu)\n", (size_t)(token - frontend->tokens));

    node_t * node = getPower(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (token->type == OPR && (token->val.op == MUL || token->val.op == DIV)){
        node_t * cur_node = token;
        token++;

        node_t * node2 = getPower(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        cur_node->left  = node;
        cur_node->right = node2;

        node = cur_node;
    }

    return node;
}

static node_t * getPower(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getPower   (token #%zu)\n", (size_t)(token - frontend->tokens));

    node_t * node = getPrimary(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (token->type == OPR && token->val.op == POW){
        node_t * cur_node = token;
        token++;

        node_t * node2 = getPower(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        cur_node->left  = node;
        cur_node->right = node2;

        node = cur_node;
    }

    return node;
}

static node_t * getPrimary(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getPrimary (token #%zu)\n", (size_t)(token - frontend->tokens));

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

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getNumber  (token #%zu)\n", (size_t)(token - frontend->tokens));

    if (token->type == NUM){
        frontend->status = SUCCESS;

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

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getVar     (token #%zu)\n", (size_t)(token - frontend->tokens));

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

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in getFunc    (token #%zu)\n", (size_t)(token - frontend->tokens));

    if (token->type != OPR){
        frontend->status = SOFT_ERROR;

        return NULL;
    }

    switch (token->val.op){
        case SIN: case COS: case LN: case TAN:{
            node_t * func_node = token;
            token++;

            if (!(token->type == OPR && token->val.op == LBRACKET)){
                frontend->status = SOFT_ERROR;
                token--;

                return NULL;
            }
            token++;

            func_node->left = getExpr(frontend);

            if (!(token->type == OPR && token->val.op == RBRACKET)){
                frontend->status = HARD_ERROR;

                return NULL;
            }
            token++;

            return func_node;
        }

        default:
            frontend->status = SOFT_ERROR;
            return NULL;
    }

    return NULL;
}

#undef token

//TODO add more functionality
static void syntaxError(const char * expected, node_t * node)
{
    assert(expected);

    fprintf(stderr, "SYNTAX ERROR\n");
}
