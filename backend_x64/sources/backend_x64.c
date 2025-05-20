#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>

#include "backend_x64.h"
#include "logger.h"
#include "IR_handler.h"


const size_t MAX_NODES_NUM = 1024;


static void makeIRrecursive(backend_ctx_t * be, node_t * cur_node);

static void IRresolveLabels(backend_ctx_t * ctx);


static name_addr_t getNameAddr(backend_ctx_t * ctx, size_t var_index);

static void addNewName(backend_ctx_t * ctx, size_t var_index, int64_t addr, bool is_global);

static void nameStackPush(backend_ctx_t * ctx, size_t var_index, int64_t addr, bool is_global);


static void enterScope(backend_ctx_t * ctx, enum scope_start scope);

static void leaveScope(backend_ctx_t * ctx, enum scope_start scope);


/************** TRANSLATORS **************/

static void translateCall(backend_ctx_t * ctx, node_t * node);

static void translateVarDecl(backend_ctx_t * ctx, node_t * node);

static void translatePushNum(backend_ctx_t * ctx, int64_t num);

static void translatePushVar(backend_ctx_t * ctx, size_t var_index);

static void translatePopVar(backend_ctx_t * ctx, size_t var_index);

static void translateCall(backend_ctx_t * ctx, node_t * node);

static void translateVarDecl(backend_ctx_t * ctx, node_t * node);

static void translateAssign(backend_ctx_t * ctx, node_t * node);

static void translateReturn(backend_ctx_t * ctx, node_t * node);

static void translateIfElse(backend_ctx_t * ctx, node_t * node);

static void translateWhile(backend_ctx_t * ctx, node_t * node);

static void translateIn(backend_ctx_t * ctx, node_t * node);

static void translateOut(backend_ctx_t * ctx, node_t * node);

static void translateAddSubMulDiv(backend_ctx_t * ctx, node_t * node);

static void translateSqrt(backend_ctx_t * ctx, node_t * node);

static void translateFuncDecl(backend_ctx_t * ctx, node_t * node);

static void translateCompare(backend_ctx_t * ctx, node_t * node);


backend_ctx_t backendInit(const char * ast_file_name)
{
    assert(ast_file_name);

    backend_ctx_t ctx = {};
    ctx.root = (node_t *)calloc(MAX_NODES_NUM, sizeof(node_t));

    tree_context_t tree = {};
    tree.cur_node = ctx.root;

    ctx.root = readTreeFromIR(&tree, ast_file_name);
    ctx.id_table_size = tree.id_size;
    ctx.id_table      = tree.ids;

    ctx.name_stack.elems = (name_addr_t *)calloc(NAME_STACK_START_CAP, sizeof(name_addr_t));
    ctx.name_stack.capacity = NAME_STACK_START_CAP;
    ctx.name_stack.size     = 0;

    ctx.in_function = false;

    return ctx;
}


void backendDestroy(backend_ctx_t * ctx)
{
    assert(ctx);

    free(ctx->id_table);
    free(ctx->root);

    ctx->id_table = NULL;
    ctx->root     = NULL;

    free(ctx->name_stack.elems);
    ctx->name_stack.elems = NULL;

    free(ctx->IR.blocks);
    ctx->IR.blocks = NULL;
}


static void nameStackPush(backend_ctx_t * ctx, size_t var_index, int64_t addr, bool is_global)
{
    if (addr > 0)
        logPrint(LOG_DEBUG_PLUS, "pushing new name: idx = %zu, %s\n", var_index, ctx->id_table[var_index].name);

    name_addr_t * cur_elem = ctx->name_stack.elems + ctx->name_stack.size;

    cur_elem->name_index = var_index;
    cur_elem->rel_addr   = addr;
    cur_elem->is_global  = is_global;

    ctx->name_stack.size++;

    // realloc if need
    if (ctx->name_stack.size >= ctx->name_stack.capacity){
        ctx->name_stack.capacity *= 2;
        ctx->name_stack.elems = (name_addr_t *)realloc(ctx->name_stack.elems, ctx->name_stack.capacity * sizeof(name_addr_t));
    }
}


static void addNewName(backend_ctx_t * ctx, size_t var_index, int64_t addr, bool is_global)
{
    nameStackPush(ctx, var_index, addr, is_global);

    if (is_global)
        ctx->global_var_counter++;
    else
        ctx->local_var_counter++;
}


static void enterScope(backend_ctx_t * ctx, enum scope_start scope)
{
    nameStackPush(ctx, scope, 0, 0);
}


static void leaveScope(backend_ctx_t * ctx, enum scope_start scope)
{
    assert(ctx->name_stack.size > 0);

    name_addr_t * elems = ctx->name_stack.elems;

    size_t start_index =  ctx->name_stack.size - 1;
    size_t elem_index = start_index;

    if (elems[elem_index].name_index == scope)
        elem_index--;
    else
        for ( ; elems[elem_index].name_index != scope; elem_index--)
            ;

    if (scope == START_OF_FUNC_SCOPE)
        ctx->local_var_counter = 0;

    else {
        ctx->local_var_counter -= (start_index - elem_index);
    }

    ctx->name_stack.size = elem_index;
}


static name_addr_t getNameAddr(backend_ctx_t * ctx, size_t var_id)
{
    assert(ctx);
    assert(ctx->name_stack.size > 0);

    for (size_t stack_index = 0; stack_index < ctx->name_stack.size; stack_index++){
        int64_t cur_id = ctx->name_stack.elems[stack_index].name_index;
        if (cur_id >= 0)
            logPrint(LOG_DEBUG_PLUS, "[%s ", ctx->id_table[cur_id].name);
        else
            logPrint(LOG_DEBUG_PLUS, "[-_- ");

        logPrint(LOG_DEBUG_PLUS, "%ld] ", cur_id);
    }
    logPrint(LOG_DEBUG_PLUS, "\n");

    name_addr_t * name_stack_elems = ctx->name_stack.elems;
    int64_t cur_name_index = 0;

    size_t search_index = ctx->name_stack.size - 1;
    logPrint(LOG_DEBUG_PLUS, "searching idx %zu\n", var_id);
    logPrint(LOG_DEBUG_PLUS, "start search_index = %zu\n", search_index);

    if (ctx->in_function){
        logPrint(LOG_DEBUG_PLUS, "searching locally\n");
        while ((cur_name_index = name_stack_elems[search_index].name_index) != START_OF_FUNC_SCOPE){
            if (cur_name_index == var_id){
                logPrint(LOG_DEBUG_PLUS, "Found! search_idx = %zu, name_index = %ld\n", search_index, cur_name_index);
                return name_stack_elems[search_index];
            }

            search_index--;
        }
    }

    logPrint(LOG_DEBUG_PLUS, "middle search_index = %zu\n", search_index);
    search_index++;

    while(search_index > 0){
        search_index--;

        cur_name_index = name_stack_elems[search_index].name_index;

        if (cur_name_index == var_id){
            logPrint(LOG_DEBUG_PLUS, "Found! search_idx = %zu, name_index = %ld\n", search_index, cur_name_index);
            return name_stack_elems[search_index];
        }
    }

    fprintf(stderr, "X64 BACKEND: ERROR: variable %s is not declared in this scope!\n",
        ctx->id_table[var_id].name);

    name_addr_t zero = {};

    return zero;
}


static IR_block_t * IRnextBlock(backend_ctx_t * ctx, enum IR_type type)
{
    if (ctx->IR.size >= ctx->IR.capacity){
        ctx->IR.capacity *= 2;
        ctx->IR.blocks = (IR_block_t *)realloc(ctx->IR.blocks, ctx->IR.capacity * sizeof(*(ctx->IR.blocks)));
    }
    size_t cur_index = ctx->IR.size;
    ctx->IR.size++;

    ctx->IR.blocks[cur_index].type = type;

    return ctx->IR.blocks + cur_index;
}


static size_t IRnewLabel(backend_ctx_t * ctx, const char * fmt, ...)
{
    IR_block_t * label_block = IRnextBlock(ctx, IR_LABEL);

    va_list args;
    va_start(args, fmt);

    vsnprintf(label_block->label_name, MAX_LABEL_NAME_LEN - 1, fmt, args);

    va_end(args);

    return label_block - ctx->IR.blocks;
}


void makeIR(backend_ctx_t * ctx)
{
    assert(ctx);

    ctx->IR.blocks = (IR_block_t *)calloc(IR_START_CAP, sizeof(*(ctx->IR.blocks)));
    ctx->IR.capacity = IR_START_CAP;
    ctx->IR.size = 0;

    makeIRrecursive(ctx, ctx->root);

    IRresolveLabels(ctx);
}


static void makeIRrecursive(backend_ctx_t * ctx, node_t * node)
{
    assert(ctx);

    if (node == NULL)
        return;

    assert(node->type == OPR);
    enum oper op_num = node->val.op;

    switch (op_num){
        case  VAR_DECL: translateVarDecl(ctx, node);    break;

        case    ASSIGN: translateAssign(ctx, node);     break;

        case    RETURN: translateReturn(ctx, node);     break;

        case        IF: translateIfElse(ctx, node);     break;

        case     WHILE: translateWhile(ctx, node);      break;

        case FUNC_DECL: translateFuncDecl(ctx, node);   break;

        case        IN: translateIn(ctx, node);         break;

        case       OUT: translateOut(ctx, node);        break;

        case SEP:
            makeIRrecursive(ctx, node->left);
            makeIRrecursive(ctx, node->right);
            break;

        default:
            fprintf(stderr, "X64 BACKEND: ERROR: invalid op_num: %d\n", op_num);
            break;
    }
}


static void IRresolveLabels(backend_ctx_t * ctx)
{
    for (size_t IR_index = 0; IR_index < ctx->IR.size; IR_index++){
        IR_block_t * block = ctx->IR.blocks + IR_index;

        if (block->type == IR_CALL){
            size_t func_index = ctx->id_table[block->name_id].IR_index;
            block->label_block_idx = func_index;
        }
    }
}



// NOTE: expressions are blocks that return something in stack
static void translateExpression(backend_ctx_t * ctx, node_t * node)
{
    assert(ctx);
    assert(node);

    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    if (node->type == NUM){
        translatePushNum(ctx, (int64_t)node->val.number);
        return;
    }

    if (node->type == IDR){
        translatePushVar(ctx, node->val.id);
        return;
    }

    assert(node->type == OPR);

    enum oper op_num = node->val.op;

    switch(op_num){
        case ADD: case SUB: case MUL: case DIV:
            translateAddSubMulDiv(ctx, node);
            break;

        case EQUAL: case N_EQUAL: case LESS: case LESS_EQ: case GREATER: case GREATER_EQ:
            translateCompare(ctx, node);
            break;

        case CALL:
            translateCall(ctx, node);
            break;

        case SQRT:
            translateSqrt(ctx, node);
            break;

        default:
            fprintf(stderr, "X64 BACKEND: ERROR: invalid op_num: %d\n", op_num);
            break;
    }
}


static void translatePushNum(backend_ctx_t * ctx, int64_t num)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    IRnextBlock(ctx, IR_PUSH_IMM)->imm_val = num;
}


static void translatePushVar(backend_ctx_t * ctx, size_t var_index)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    name_addr_t var_addr = getNameAddr(ctx, var_index);
    IRnextBlock(ctx, IR_PUSH_MEM)->var = var_addr;
}

static void translatePopVar(backend_ctx_t * ctx, size_t var_index)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    name_addr_t var_addr = getNameAddr(ctx, var_index);
    IRnextBlock(ctx, IR_POP_MEM)->var = var_addr;
}


static void translateAddSubMulDiv(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->left);
    translateExpression(ctx, node->right);

    enum oper op_num = node->val.op;

    switch(op_num){
        case ADD: IRnextBlock(ctx, IR_ADD); break;
        case SUB: IRnextBlock(ctx, IR_SUB); break;
        case MUL: IRnextBlock(ctx, IR_MUL); break;
        case DIV: IRnextBlock(ctx, IR_DIV); break;
    }
}

static void translateSqrt(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->left);

    IRnextBlock(ctx, IR_SQRT);
}


static void translateCallHandleArgs(backend_ctx_t * ctx, node_t * arg_node);

static void translateCall(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    node_t * func_node = node->left;
    node_t * arg_tree  = node->right;

    size_t num_of_args = ctx->id_table[func_node->val.id].num_of_args;
    const char * func_name = ctx->id_table[func_node->val.id].name;

    translateCallHandleArgs(ctx, arg_tree);

    IRnextBlock(ctx, IR_CALL)->name_id = func_node->val.id;
    //!!! label_block_idx field must be set later!!!
}

static void translateCallHandleArgs(backend_ctx_t * ctx, node_t * arg_node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    if (arg_node == NULL)
        return;

    translateCallHandleArgs(ctx, arg_node->right);
    translateExpression(ctx, arg_node->left);
}


static void translateVarDecl(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    int64_t addr = (ctx->in_function) ?
        (- ctx->local_var_counter  * 8 - 8):
        (- ctx->global_var_counter * 8);

    printf("name = %s, addr = %ld, global = %d\n", ctx->id_table[node->left->val.id].name, addr, !(ctx->in_function));

    size_t var_index = node->left->val.id;
    addNewName(ctx, var_index, addr, !(ctx->in_function));

    IR_block_t * var_block = IRnextBlock(ctx, IR_VAR_DECL);
    var_block->var  = getNameAddr(ctx, var_index);
}


static void translateAssign(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->right);
    translatePopVar(ctx, node->left->val.id);
}


static void translateReturn(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->left);
    (void)IRnextBlock(ctx, IR_RET);
}


static void translateIfElse(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // condition expression
    translateExpression(ctx, node->left);

    enterScope(ctx, START_OF_SCOPE);

    size_t if_counter = ctx->if_counter;
    ctx->if_counter++;

    if (node->right->type == OPR && node->right->val.op == IF_ELSE){
        // we have else
        node_t * if_else_node = node->right;

        // condition
        IR_block_t * else_cond_jmp = IRnextBlock(ctx, IR_COND_JMP);

        // if body
        makeIRrecursive(ctx, if_else_node->left);

        // jump over else
        IR_block_t * jmp_over_else = IRnextBlock(ctx, IR_JMP);

        // else label
        else_cond_jmp->label_block_idx = IRnewLabel(ctx, "__IF_%zu_ELSE", if_counter);

        // else body
        makeIRrecursive(ctx, if_else_node->right);

        // end label
        jmp_over_else->label_block_idx = IRnewLabel(ctx, "__IF_%zu_END", if_counter);
    }
    else {
        // we do not have else

        // condition
        IR_block_t * end_cond_jmp = IRnextBlock(ctx, IR_COND_JMP);

        makeIRrecursive(ctx, node->right);

        // end label
        end_cond_jmp->label_block_idx = IRnewLabel(ctx, "__IF_%zu_END", if_counter);
    }

    leaveScope(ctx, START_OF_SCOPE);
}


static void translateWhile(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // label of loop start
    size_t cond_check_label_idx = IRnewLabel(ctx, "__WHILE_%zu_COND_CHECK:\n", ctx->while_counter);

    // condition expression
    translateExpression(ctx, node->left);

    enterScope(ctx, START_OF_SCOPE);

    size_t while_counter = ctx->while_counter;
    ctx->while_counter++;

    // cond jump block
    IR_block_t * cond_jmp_to_end = IRnextBlock(ctx, IR_COND_JMP);

    // body of while
    makeIRrecursive(ctx, node->right);

    // jmp to the start
    IR_block_t * jmp_to_cond_check = IRnextBlock(ctx, IR_JMP);
    jmp_to_cond_check->label_block_idx = cond_check_label_idx;

    // end label
    size_t end_label_idx = IRnewLabel(ctx, "__WHILE_%zu_END\n", ctx->while_counter);
    cond_jmp_to_end->label_block_idx = end_label_idx;

    leaveScope(ctx, START_OF_SCOPE);
    ctx->while_counter++;
}


static void translateFuncDecl(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    node_t * func_head = node->left;
    node_t * func_body = node->right;

    node_t * func_node = func_head->left;
    node_t * func_arg  = func_head->right;

    const char * func_name = ctx->id_table[func_node->val.id].name;

    size_t num_of_args = ctx->id_table[func_node->val.id].num_of_args;

    enterScope(ctx, START_OF_FUNC_SCOPE);
    ctx->in_function = true;

    for (size_t arg_index = 0; arg_index < num_of_args; arg_index++){
        nameStackPush(ctx, func_arg->left->val.id, arg_index * 8 + 16, false);

        func_arg = func_arg->right;
    }


    // jump over function
    IR_block_t * jmp_over_func = IRnextBlock(ctx, IR_JMP);

    // label index of the function
    size_t func_label_idx = IRnewLabel(ctx, "%s", func_name);
    ctx->id_table[func_node->val.id].IR_index = func_label_idx; //into the table
    ctx->IR.blocks[func_label_idx].arg_num = num_of_args;


    // setting frame pointer
    IRnextBlock(ctx, IR_SET_FR_PTR);

    // func body
    makeIRrecursive(ctx, func_body);

    // end label
    jmp_over_func->label_block_idx = IRnewLabel(ctx, "__END_OF_%s__", func_name);

    leaveScope(ctx, START_OF_FUNC_SCOPE);
    ctx->in_function = false;
}


static void translateIn(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    name_addr_t var_addr = getNameAddr(ctx, node->left->val.id);

    IRnextBlock(ctx, IR_IN)->var = var_addr;
}


static void translateOut(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // pushing arg
    translateExpression(ctx, node->left);

    IRnextBlock(ctx, IR_OUT);
}


static void translateCompare(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->left);
    translateExpression(ctx, node->right);

    switch (node->val.op){
        case EQUAL:      IRnextBlock(ctx, IR_EQUAL); break;
        case N_EQUAL:    IRnextBlock(ctx, IR_N_EQUAL); break;
        case LESS:       IRnextBlock(ctx, IR_LESS); break;
        case GREATER:    IRnextBlock(ctx, IR_GREATER); break;
        case LESS_EQ:    IRnextBlock(ctx, IR_LESS_EQ); break;
        case GREATER_EQ: IRnextBlock(ctx, IR_GREATER_EQ); break;
    }
}
