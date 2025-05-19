#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>

#include "backend_x64.h"
#include "logger.h"
#include "IR_handler.h"


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

    size_t elem_index = ctx->name_stack.size - 1;
    for ( ; elems[elem_index].name_index != scope; elem_index--)
        ;

    ctx->name_stack.size = elem_index;
}


static name_addr_t * getNameAddr(backend_ctx_t * ctx, size_t var_index)
{
    assert(ctx);
    assert(ctx->name_stack.size != 0);

    name_addr_t * name_stack_elems = ctx->name_stack.elems;
    int64_t cur_name_index = 0;

    size_t search_index = ctx->name_stack.size;
    logPrint(LOG_DEBUG_PLUS, "search_index = %zu\n", search_index);

    if (ctx->in_function){
        while ((cur_name_index = name_stack_elems[search_index].name_index) != START_OF_FUNC_SCOPE){
            search_index--;

            if (cur_name_index == var_index)
                return name_stack_elems + search_index + 1;
        }
    }

    while(search_index > 0){
        search_index--;

        cur_name_index = name_stack_elems[search_index].name_index;

        if (cur_name_index == var_index)
            return name_stack_elems + search_index;
    }

    fprintf(stderr, "X64 BACKEND: ERROR: variable %s is not declared in this scope!\n",
        ctx->id_table[var_index].name);

    return NULL;
}


static IR_block_t * IRnextBlock(backend_ctx_t * ctx, enum IR_type type)
{
    if (ctx->IR.size >= ctx->IR.capacity){
        ctx->IR.capacity *= 2;
        ctx->IR.blocks = (IR_block_t *)realloc(ctx->IR.capacity, sizeof(*(ctx->IR.blocks)));
    }

    ctx->IR.size++;
    ctx->IR.blocks[ctx->IR.size].type = type;

    return ctx->IR.blocks + ctx->IR.size;
}


static size_t IRnewLabel(backend_ctx_t * ctx, const char * fmt, ...)
{
    IR_block_t * label_block = IRnextBlock(ctx);
    label_block->type = IR_LABEL;

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

    makeAssemblyIRrecursive(ctx, ctx->root);
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

        case CALL:
            translateCall(ctx, node);
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

    name_addr_t * var_addr = getNameAddr(ctx, var_index);
    IRnextBlock(ctx, IR_PUSH_MEM)->var = var_addr;
}

static void translatePopVar(backend_ctx_t * ctx, size_t var_index)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    name_addr_t * var_addr = getNameAddr(ctx, var_index);
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


static void translateCallHandleArgs(backend_ctx_t * ctx, node_t * arg_node);

static void translateCall(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    node_t * func_node = node->left;
    node_t * arg_tree  = node->right;

    size_t num_of_args = ctx->id_table[func_node->val.id].num_of_args;
    const char * func_name = ctx->id_table[func_node->val.id].name;

    translateCallHandleArgs(ctx, arg_tree);

    IRnextBlock(ctx, IR_CALL);
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

    printf("name = %s, addr = %ld\n", ctx->id_table[node->left->val.id].name, addr);

    size_t var_index = node->left->val.id;
    addNewName(ctx, var_index, addr, !(ctx->in_function));

    IR_block_t * next_block = IRnextBlock(ctx, IR_VAR_DECL);
    next_block->var  = getNameAddr(ctx, var_index);
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

    (void)IRnextBlock(ctx, IR_RET);
}


static void translateIfElse(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // condition expression
    translateExpression(ctx, node->left);

    enterScope(ctx, START_OF_SCOPE);

    if (node->right->type == OPR && node->right->val.op == IF_ELSE){
        // we have else
        node_t * if_else_node = node->right;

        // condition
        IR_block_t * else_cond_jmp = IRnextBlock(ctx);
        else_cond_jmp->type = IR_COND_JMP;

        // if body
        makeIRrecursive(ctx, if_else_node->right);

        // jump over else
        IR_block_t * jmp_over_else = IRnextBlock(ctx);
        jmp_over_else->type = IR_JMP;

        // else label
        else_cond_jmp->label_block_idx = IRnewLabel(ctx, "__IF_%zu_ELSE", ctx->if_counter);

        // else body
        makeIRrecursive(ctx, if_else_node->left);

        // end label
        jmp_over_else->label_block_idx = IRnewLabel(ctx, "__IF_%zu_END:", ctx->if_counter);
    }
    else {
        // we do not have else

        // condition
        IR_block_t * end_cond_jmp = IRnextBlock(ctx);
        end_cond_jmp->type = IR_COND_JMP;

        makeIRrecursive(ctx, node->right);

        // end label
        end_cond_jmp->label_block_idx = IRnewLabel(ctx, "__IF_%zu_END:", ctx->if_counter);
    }

    leaveScope(ctx, START_OF_SCOPE);
    ctx->if_counter++;
}


static void translateWhile(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // label of loop start
    size_t cond_check_label_idx = IRnewLabel(ctx, "__WHILE_%zu_COND_CHECK:\n", ctx->while_counter);

    // condition expression
    translateExpression(ctx, node->left);

    enterScope(ctx, START_OF_SCOPE);

    // cond jump block
    IR_block_t * cond_jmp_to_end = IRnextBlock(ctx, IR_COND_JMP);

    // body of while
    makeIRrecursive(ctx, node->right);

    // jmp to the start
    IR_block_t * jmp_to_cond_check = IRnextBlock(ctx, IR_JMP);
    jmp_to_cond_check->label_block_idx = cond_check_label_idx;

    // end label
    size_t end_label_idx = IRnewLabel(ctx, "__WHILE_%zu_END:\n", ctx->while_counter);
    cond_jmp_to_end->label_block_idx = end_label_idx;

    leaveScope(ctx, START_OF_SCOPE);
    ctx->while_counter++;
}


static void translateFuncDeclHandleArgs(backend_ctx_t * ctx, node_t * node, int64_t addr);

static void translateFuncDecl(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    node_t * func_head = node->left;
    node_t * func_body = node->right;

    node_t * func_node = func_head->left;
    node_t * func_args = func_head->right;

    const char * func_name = ctx->id_table[func_node->val.id].name;

    size_t num_of_args = ctx->id_table[func_node->val.id].num_of_args;
    translateFuncDeclHandleArgs(ctx, func_args, num_of_args * 8 + 8);

    enterScope(ctx, START_OF_FUNC_SCOPE);
    ctx->in_function = true;

    // jump over function
    IR_block_t * jmp_over_func = IRnextBlock(ctx, IR_JMP);

    // setting frame pointer
    IRnextBlock(ctx, IR_SET_FR_PTR);

    // func body
    makeIRrecursive(ctx, func_body);

    // end label
    jmp_over_func->label_block_idx = IRnewLabel(ctx, "__END_OF_%s__:\n", func_name);

    leaveScope(ctx, START_OF_FUNC_SCOPE);
    ctx->in_function = false;
    ctx->local_var_counter = 0;
}

static void translateFuncDeclHandleArgs(backend_ctx_t * ctx, node_t * node, int64_t addr)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    if (node == NULL)
        return;

    translateFuncDeclHandleArgs(ctx, node->right, addr + 8);
    nameStackPush(ctx, node->left->val.id, addr, false);
}


static void translateIn(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    name_addr_t * var_addr = getNameAddr(ctx, node->left->val.id);

    IRnextBlock(ctx, IR_IN)->var = var_addr;
}


static void translateOut(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // pushing arg
    translateExpression(ctx, node->left);

    IRnextBlock(ctx, IR_OUT);
}

