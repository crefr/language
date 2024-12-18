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
    IS_IDR,
    IS_OPR,
    IS_IDR_OR_OPR
};

static enum id_stat getIdName(const char ** src_str, char * buffer);

static void skipSpaces(const char ** code);

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


/*----------------------recursive descent----------------------*/

typedef node_t * (* syntax_func_t)(fe_context_t * frontend);

static node_t * getChain(fe_context_t * frontend);

static node_t * getStatement(fe_context_t * frontend);
static node_t * getBlock(fe_context_t * frontend);

static node_t * getFuncDecl(fe_context_t * frontend);
static node_t * getFuncCall(fe_context_t * frontend);

static node_t * getIF(fe_context_t * frontend);
static node_t * getWhile(fe_context_t * frontend);

static node_t * getAssign(fe_context_t * frontend);
static node_t * getSTDfunc(fe_context_t * frontend);

static node_t * getInput(fe_context_t * frontend);
static node_t * getOutput(fe_context_t * frontend);

static node_t * getReturn(fe_context_t * frontend);

static node_t * getExpr(fe_context_t * frontend);
static node_t * getMulDiv(fe_context_t * frontend);
static node_t * getPower(fe_context_t * frontend);
static node_t * getPrimary(fe_context_t * frontend);

static node_t * getNumber(fe_context_t * frontend);

static node_t * getMathFunc(fe_context_t * frontend);
static node_t * getId(fe_context_t * frontend);

static void syntaxError(const char * expected, node_t * node);

#define token (frontend->cur_node)

#define LBRACKET_SKIP                                                   \
    do {                                                                \
        if (!(token->type == OPR && token->val.op == LBRACKET)){        \
            frontend->status = HARD_ERROR;                              \
            syntaxError("(", token);                                    \
                                                                        \
            return NULL;                                                \
        }                                                               \
        token++;                                                        \
    } while(0)

#define RBRACKET_SKIP                                                   \
    do {                                                                \
        if (!(token->type == OPR && token->val.op == RBRACKET)){        \
            frontend->status = HARD_ERROR;                              \
            syntaxError(")", token);                                    \
                                                                        \
            return NULL;                                                \
        }                                                               \
        token++;                                                        \
    } while(0)

#define tokenisOPR(op_num)  (token->type == OPR && token->val.op == op_num)

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

static node_t * getBlock(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    if (! tokenisOPR(BEGIN)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    token++;

    node_t * block_tree = getChain(frontend);
    if (block_tree == NULL){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    if (! tokenisOPR(ENDING)){
        frontend->status = HARD_ERROR;
        return NULL;
    }
    token++;

    frontend->status = SUCCESS;

    return block_tree;
}

static node_t * getChain(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    node_t * cur = getStatement(frontend);

    if (!(token->type == OPR && token->val.op == SEP)){
        frontend->status = HARD_ERROR;

        return NULL;
    }

    node_t * tree = token;
    tree->left = cur;
    token++;

    node_t * last = tree;

    while ((cur = getStatement(frontend)) != NULL){
        if (!(token->type == OPR && token->val.op == SEP)){
            frontend->status = HARD_ERROR;

            return NULL;
        }

        token->left = cur;
        token->right = NULL;

        last->right = token;

        last = token;

        token++;
    }

    return tree;
}

static node_t * getStatement(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    const syntax_func_t statement_funcs[] = {
        getSTDfunc,
        getIF,
        getWhile,
        getFuncDecl,
        getAssign,
        getReturn
    };
    const size_t num_of_funcs = sizeof(statement_funcs) / sizeof(statement_funcs[0]);

    for (size_t func_index = 0; func_index < num_of_funcs; func_index++){
        node_t * tree = statement_funcs[func_index](frontend);

        if (frontend->status == SUCCESS)
            return tree;

        if (frontend->status == HARD_ERROR)
            return NULL;
    }

    return NULL;
}

static node_t * getWhile(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    frontend->status = SUCCESS;

    if (! tokenisOPR(WHILE)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }

    node_t * while_node = token;
    token++;

    LBRACKET_SKIP;

    node_t * cond_tree = getExpr(frontend);
    if (frontend->status != SUCCESS){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    RBRACKET_SKIP;

    node_t * body_tree = getBlock(frontend);
    if (body_tree == NULL){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    while_node->left  = cond_tree;
    while_node->right = body_tree;

    frontend->status = SUCCESS;

    return while_node;
}

static node_t * getFuncDecl(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    frontend->status = SUCCESS;

    if (! tokenisOPR(FUNC_DECL)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    node_t * func = token;
    token++;

    if (token->type != IDR){
        frontend->status = HARD_ERROR;
        return NULL;
    }
    node_t * id_node = token;
    token++;

    // setting type to FUNC in name table
    frontend->ids[id_node->val.id].type = FUNC;

    // we will use this node later
    node_t * arg_tree = token;
    LBRACKET_SKIP;

    // if there are no arguments given
    if (token->type != IDR){
        // so we do not actually need arg tree
        arg_tree = NULL;
    }
    else {
        // so we have at least one argument

        // transforming left bracket into ARG_SEP
        arg_tree->type = OPR;
        arg_tree->val.op = ARG_SEP;

        // first arg handling
        if (token->type != IDR){
            frontend->status = HARD_ERROR;
            return NULL;
        }
        node_t * first_arg_node = token;
        token++;

        arg_tree->left  = first_arg_node;
        arg_tree->right = NULL;

        // handling other args
        node_t * last_sep = arg_tree;
        node_t * cur_sep  = token;

        while (cur_sep->type == OPR && cur_sep->val.op == ARG_SEP){
            token++;
            if (token->type != IDR){
                frontend->status = HARD_ERROR;
                return NULL;
            }
            node_t * cur_arg_node = token;
            token++;

            last_sep->right = cur_sep;

            cur_sep->left  = cur_arg_node;
            cur_sep->right = NULL;

            last_sep = cur_sep;

            cur_sep = token;
        }
    }

    // we will use this node later
    node_t * func_header = token;
    RBRACKET_SKIP;

    // transforming right bracket into a FUNC_HEADER
    func_header->type = OPR;
    func_header->val.op = FUNC_HEADER;

    func_header->left = id_node;

    // there is arg tree on the right
    func_header->right = arg_tree;

    node_t * body_tree = getBlock(frontend);
    if (frontend->status != SUCCESS){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    func->left  = func_header;
    func->right = body_tree;

    return func;
}

static node_t * getIF(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    frontend->status = SUCCESS;

    if (! tokenisOPR(IF)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }

    node_t * if_node = token;
    token++;

    LBRACKET_SKIP;

    node_t * cond_tree = getExpr(frontend);
    if (frontend->status != SUCCESS){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    RBRACKET_SKIP;

    node_t * body_tree = getBlock(frontend);
    if (body_tree == NULL){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    if_node->left  = cond_tree;
    if_node->right = body_tree;

    frontend->status = SUCCESS;

    return if_node;
}

// TODO: getInput, getOutput etc. can be implemented in one function with table of standard functions
static node_t * getSTDfunc(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    node_t * func = getInput(frontend);

    if (frontend->status == SUCCESS)
        return func;
    if (frontend->status == HARD_ERROR)
        return NULL;

    func = getOutput(frontend);

    if (frontend->status == SUCCESS)
        return func;
    if (frontend->status == HARD_ERROR)
        return NULL;

    return NULL;
}

static node_t * getInput(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    frontend->status = SUCCESS;

    if (!(token->type == OPR && token->val.op == IN)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }

    node_t * input_node = token;
    token++;

    LBRACKET_SKIP;

    node_t * var_node = getId(frontend);
    if (frontend->status != SUCCESS)
        return NULL;

    RBRACKET_SKIP;

    input_node->left  = var_node;
    input_node->right = NULL;

    return input_node;
}

static node_t * getOutput(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    frontend->status = SUCCESS;

    if (!(token->type == OPR && token->val.op == OUT)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }

    node_t * output_node = token;
    token++;

    LBRACKET_SKIP;

    node_t * expr_tree = getExpr(frontend);
    if (frontend->status != SUCCESS)
        return NULL;

    RBRACKET_SKIP;

    output_node->left  = expr_tree;
    output_node->right = NULL;

    return output_node;
}

static node_t * getReturn(fe_context_t * frontend)
{
    assert(frontend);

    frontend->status = SUCCESS;

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    node_t * ret_node = token;
    if (! tokenisOPR(RETURN)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    token++;

    node_t * expr_tree = getExpr(frontend);
    if (frontend->status != SUCCESS){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    ret_node->left = expr_tree;

    return ret_node;
}

static node_t * getAssign(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    node_t * left_part = getId(frontend);
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

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

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

    frontend->status = SUCCESS;

    return node;
}

static node_t * getMulDiv(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

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

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

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

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

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

    node_t * math_func = getMathFunc(frontend);
    if (frontend->status == SUCCESS)
        return math_func;
    else if (frontend->status == HARD_ERROR)
        return NULL;

    node_t * func = getFuncCall(frontend);
    if (frontend->status == SUCCESS)
        return func;
    else if (frontend->status == HARD_ERROR)
        return NULL;

    node_t * id = getId(frontend);
    if (frontend->status == SUCCESS)
        return id;
    else if (frontend->status == HARD_ERROR)
        return NULL;

    node_t * number = getNumber(frontend);
    if (frontend->status == SUCCESS)
        return number;

    return NULL;
}

static node_t * getFuncCall(fe_context_t * frontend)
{
    assert(frontend);

    frontend->status = SUCCESS;

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    if (token->type != IDR){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    node_t * func_node = token;
    token++;

    if (! tokenisOPR(LBRACKET)){
        frontend->status = SOFT_ERROR;
        token--;

        return NULL;
    }

    // we will transform this node into ARG_SEP
    node_t * arg_tree = token;
    token++;

    // it has left bracket so it is a function
    frontend->ids[func_node->val.id].type = FUNC;

    // if there are no arguments given
    if (token->type != IDR){
        // so we do not actually need arg tree
        arg_tree = NULL;
    }
    else {
        // so we have at least one argument

        // transforming left bracket into ARG_SEP
        arg_tree->type = OPR;
        arg_tree->val.op = ARG_SEP;

        // first arg handling
        if (token->type != IDR){
            frontend->status = HARD_ERROR;
            return NULL;
        }
        node_t * first_arg_node = token;
        token++;

        arg_tree->left  = first_arg_node;
        arg_tree->right = NULL;

        // handling other args
        node_t * last_sep = arg_tree;
        node_t * cur_sep  = token;

        while (cur_sep->type == OPR && cur_sep->val.op == ARG_SEP){
            token++;
            if (token->type != IDR){
                frontend->status = HARD_ERROR;
                return NULL;
            }
            node_t * cur_arg_node = token;
            token++;

            last_sep->right = cur_sep;

            cur_sep->left  = cur_arg_node;
            cur_sep->right = NULL;

            last_sep = cur_sep;

            cur_sep = token;
        }
    }

    node_t * call_node = token;
    RBRACKET_SKIP;

    call_node->val.op = CALL;
    call_node->left = func_node;

    // right subtree is arg tree
    call_node->right = arg_tree;

    return call_node;
}

static node_t * getNumber(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    if (token->type == NUM){
        frontend->status = SUCCESS;

        node_t * num_node = token;
        token++;

        return num_node;
    }
    frontend->status = HARD_ERROR;

    return NULL;
}

static node_t * getId(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

    if (token->type != IDR){
        frontend->status  = SOFT_ERROR;

        return NULL;
    }
    frontend->status = SUCCESS;

    node_t * id_node = token;
    token++;

    return id_node;
}

static node_t * getMathFunc(fe_context_t * frontend)
{
    assert(frontend);

    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens));

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

#undef RBRACKET_SKIP
#undef LBRACKET_SKIP
#undef token

//TODO add more functionality
static void syntaxError(const char * expected, node_t * node)
{
    assert(expected);

    fprintf(stderr, "SYNTAX ERROR\n");
}
