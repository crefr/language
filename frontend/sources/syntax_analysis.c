#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "logger.h"
#include "tree.h"
#include "frontend.h"

typedef node_t * (* syntax_func_t)(fe_context_t * frontend);

static node_t * getChain(fe_context_t * frontend);

static node_t * getStatement(fe_context_t * frontend);
static node_t * getBlock(fe_context_t * frontend);

static node_t * getFuncDecl(fe_context_t * frontend);
static node_t * getFuncCall(fe_context_t * frontend);

static node_t * getVarDecl(fe_context_t * frontend);

static node_t * getIF(fe_context_t * frontend);
static node_t * getElse(fe_context_t * frontend);

static node_t * getWhile(fe_context_t * frontend);

static node_t * getAssign(fe_context_t * frontend);
static node_t * getSTDfunc(fe_context_t * frontend);

static node_t * getInput(fe_context_t * frontend);
static node_t * getOutput(fe_context_t * frontend);

static node_t * getReturn(fe_context_t * frontend);

static node_t * getExpr(fe_context_t * frontend);
static node_t * getAddSub(fe_context_t * frontend);
static node_t * getMulDiv(fe_context_t * frontend);
static node_t * getPower(fe_context_t * frontend);
static node_t * getPrimary(fe_context_t * frontend);

static node_t * getNumber(fe_context_t * frontend);

static node_t * getMathFunc(fe_context_t * frontend);
static node_t * getId(fe_context_t * frontend);

static void syntaxError(fe_context_t * fe, const char * expected, node_t * node);

#define LOG_SYNTAX_FUNC_INFO       \
    logPrint(LOG_DEBUG_PLUS, "SYNTAX: in %-20s (token #%zu)\n", __FUNCTION__, (size_t)(token - frontend->tokens))


#define token (frontend->cur_node)

#define SYNTAX_ERROR(expected)              \
    do {                                    \
        frontend->status = HARD_ERROR;      \
        syntaxError(frontend, expected, token);       \
                                            \
        return NULL;                        \
    } while(0)

#define LBRACKET_SKIP                                                   \
    do {                                                                \
        if (!(token->type == OPR && token->val.op == LBRACKET)){        \
            SYNTAX_ERROR("(");                                          \
        }                                                               \
        token++;                                                        \
    } while(0)

#define RBRACKET_SKIP                                                   \
    do {                                                                \
        if (!(token->type == OPR && token->val.op == RBRACKET)){        \
            SYNTAX_ERROR(")");                                          \
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

    LOG_SYNTAX_FUNC_INFO;

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
        SYNTAX_ERROR("end");
    }
    token++;

    frontend->status = SUCCESS;

    return block_tree;
}

static node_t * getChain(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

    node_t * cur = getStatement(frontend);
    if (frontend->status == HARD_ERROR)
        return NULL;

    if (!(token->type == OPR && token->val.op == SEP)){
        SYNTAX_ERROR(";");
    }

    node_t * tree = token;
    tree->left = cur;
    token++;

    node_t * last = tree;

    while ((cur = getStatement(frontend)) != NULL){
        if (!(token->type == OPR && token->val.op == SEP)){
            SYNTAX_ERROR(";");
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

    LOG_SYNTAX_FUNC_INFO;

    const syntax_func_t statement_funcs[] = {
        getSTDfunc,
        getIF,
        getWhile,
        getFuncDecl,
        getVarDecl,
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

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

    frontend->status = SUCCESS;

    if (! tokenisOPR(FUNC_DECL)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    node_t * func = token;
    token++;

    if (token->type != IDR){
        SYNTAX_ERROR("identifier");
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
        frontend->ids[id_node->val.id].num_of_args = 0;
    }
    else {
        // so we have at least one argument

        // transforming left bracket into ARG_SEP
        arg_tree->type = OPR;
        arg_tree->val.op = ARG_SEP;

        // first arg handling
        if (token->type != IDR){
            SYNTAX_ERROR("identifier");
        }
        node_t * first_arg_node = token;
        token++;

        frontend->ids[id_node->val.id].num_of_args = 1;

        arg_tree->left  = first_arg_node;
        arg_tree->right = NULL;

        // handling other args
        node_t * last_sep = arg_tree;
        node_t * cur_sep  = token;

        while (cur_sep->type == OPR && cur_sep->val.op == ARG_SEP){
            token++;
            if (token->type != IDR){
                SYNTAX_ERROR("identifier");
            }
            node_t * cur_arg_node = token;
            token++;

            frontend->ids[id_node->val.id].num_of_args++;

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

static node_t * getElse(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

    frontend->status = SUCCESS;

    if (! tokenisOPR(IF_ELSE)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    node_t * if_else_node = token;
    token++;

    node_t * else_body_tree = getBlock(frontend);
    if (else_body_tree == NULL){
        SYNTAX_ERROR("else body");
    }

    // on the left should be IF body
    if_else_node->left  = NULL;
    if_else_node->right = else_body_tree;

    frontend->status = SUCCESS;

    return if_else_node;
}

static node_t * getIF(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

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

    node_t * if_body_tree = getBlock(frontend);
    if (if_body_tree == NULL){
        frontend->status = HARD_ERROR;
        return NULL;
    }

    node_t * else_body_tree = getElse(frontend);
    if (frontend->status == HARD_ERROR)
        return NULL;

    frontend->status = SUCCESS;

    if_node->left  = cond_tree;

    if (else_body_tree == NULL){
        if_node->right = if_body_tree;
    }
    else {
        else_body_tree->left = if_body_tree;

        if_node->right = else_body_tree;
    }

    return if_node;
}

static node_t * getSTDfunc(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

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
        SYNTAX_ERROR("identifier");

    RBRACKET_SKIP;

    input_node->left  = var_node;
    input_node->right = NULL;

    return input_node;
}

static node_t * getOutput(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

    node_t * left_part = getId(frontend);
    if (frontend->status != SUCCESS){
        return NULL;
    }

    node_t * assign_node = token;
    if (!(token->type == OPR && token->val.op == ASSIGN)){
        SYNTAX_ERROR("=");
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

    LOG_SYNTAX_FUNC_INFO;

    node_t * node = getAddSub(frontend);

    if (frontend->status == HARD_ERROR)
        return NULL;

    while (tokenisOPR(GREATER) || tokenisOPR(LESS) || tokenisOPR(LESS_EQ) || tokenisOPR(GREATER_EQ) || tokenisOPR(EQUAL) || tokenisOPR(N_EQUAL)){
        node_t * cur_node = token;
        token++;

        node_t * node2 = getAddSub(frontend);

        if (frontend->status == HARD_ERROR)
            return NULL;

        cur_node->left  = node;
        cur_node->right = node2;

        node = cur_node;
    }

    frontend->status = SUCCESS;

    return node;
}

static node_t * getAddSub(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

    if (token->type == OPR && token->val.op == LBRACKET){
        token++;

        node_t * node = getExpr(frontend);

        RBRACKET_SKIP;

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

static node_t * getVarDecl(fe_context_t * frontend)
{
    assert(frontend);

    frontend->status = SUCCESS;

    LOG_SYNTAX_FUNC_INFO;

    if (! tokenisOPR(VAR_DECL)){
        frontend->status = SOFT_ERROR;
        return NULL;
    }
    node_t * decl_node = token;
    token++;

    if (token->type != IDR){
        SYNTAX_ERROR("identifier");
    }
    node_t * id_node = token;
    token++;

    frontend->ids[id_node->val.id].type = VAR;
    decl_node->left = id_node;

    return decl_node;
}

static node_t * getFuncCall(fe_context_t * frontend)
{
    assert(frontend);

    frontend->status = SUCCESS;

    LOG_SYNTAX_FUNC_INFO;

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
    if (tokenisOPR(RBRACKET)){
        // so we do not actually need arg tree
        arg_tree = NULL;
    }
    else {
        // so we have at least one argument

        // transforming left bracket into ARG_SEP
        arg_tree->type = OPR;
        arg_tree->val.op = ARG_SEP;

        // first arg handling
        node_t * first_arg_node = getExpr(frontend);
        if (frontend->status != SUCCESS){
            frontend->status = HARD_ERROR;
            return NULL;
        }

        arg_tree->left  = first_arg_node;
        arg_tree->right = NULL;

        // handling other args
        node_t * last_sep = arg_tree;
        node_t * cur_sep  = token;

        while (cur_sep->type == OPR && cur_sep->val.op == ARG_SEP){
            token++;

            node_t * cur_arg_node = getExpr(frontend);
            if (frontend->status != SUCCESS){
                frontend->status = HARD_ERROR;
                return NULL;
            }

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

    LOG_SYNTAX_FUNC_INFO;

    if (token->type == NUM){
        frontend->status = SUCCESS;

        node_t * num_node = token;
        token++;

        return num_node;
    }
    SYNTAX_ERROR("number");

    return NULL;
}

static node_t * getId(fe_context_t * frontend)
{
    assert(frontend);

    LOG_SYNTAX_FUNC_INFO;

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

    LOG_SYNTAX_FUNC_INFO;

    if (token->type != OPR){
        frontend->status = SOFT_ERROR;

        return NULL;
    }

    switch (token->val.op){
        case SIN: case COS: case LN: case TAN: case SQRT:{
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

static void syntaxError(fe_context_t * fe, const char * expected, node_t * node)
{
    assert(expected);

    const char * what_got = NULL;

    if (node->type == OPR)
        what_got = opers[node->val.op].name;
    else if (node->type == IDR)
        what_got = fe->ids[node->val.id].name;
    else
        what_got = "NUMBER";

    fprintf(stderr, "SYNTAX ERROR: expected %s, but got '%s'\n", expected, what_got);
}
