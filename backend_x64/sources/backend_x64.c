#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "backend_x64.h"
#include "logger.h"
#include "IR_handler.h"


const size_t MAX_NODES_NUM = 1024;


static void makeAssemblyCodeRecursive(backend_ctx_t * be, node_t * cur_node);


static name_addr_t * getNameAddr(backend_ctx_t * ctx, size_t var_index);

static void addNewName(backend_ctx_t * ctx, size_t var_index, int64_t addr, bool is_global);

static void nameStackPush(backend_ctx_t * ctx, size_t var_index, int64_t addr, bool is_global);


static void enterScope(backend_ctx_t * ctx, enum scope_start scope);

static void leaveScope(backend_ctx_t * ctx, enum scope_start scope);


/************** TRANSLATORS **************/
static void emitStart(backend_ctx_t * ctx);

static void emitExit(backend_ctx_t * ctx);


static void translateCall(backend_ctx_t * ctx, node_t * node);

static void translateVarDecl(backend_ctx_t * ctx, node_t * node);

static void translatePushNum(backend_ctx_t * ctx, int64_t num);

static void translatePushVar(backend_ctx_t * ctx, size_t var_index);

static void translateCall(backend_ctx_t * ctx, node_t * node);

static void translateVarDecl(backend_ctx_t * ctx, node_t * node);

static void translateAssign(backend_ctx_t * ctx, node_t * node);

static void translateReturn(backend_ctx_t * ctx, node_t * node);

static void translateIfElse(backend_ctx_t * ctx, node_t * node);

static void translateWhile(backend_ctx_t * ctx, node_t * node);

static void translateIn(backend_ctx_t * ctx, node_t * node);

static void translateOut(backend_ctx_t * ctx, node_t * node);

static void translateAddSubMulDiv(backend_ctx_t * ctx, node_t * node);

static void translateFuncDecl(backend_ctx_t * ctx, node_t * node);


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


void makeAssemblyCode(backend_ctx_t * ctx, const char * asm_file_name)
{
    assert(ctx);
    assert(asm_file_name);

    ctx->asm_file = fopen(asm_file_name, "w");
    logPrint(LOG_DEBUG, "\nstarted translating to asm...\n");

    emitStart(ctx);

    makeAssemblyCodeRecursive(ctx, ctx->root);

    emitExit(ctx);

    logPrint(LOG_DEBUG, "\nsuccessfully translated to asm!\n");
    fclose(ctx->asm_file);
}

#define asm_emit(...) fprintf(ctx->asm_file, __VA_ARGS__)


static void emitStart(backend_ctx_t * ctx)
{
    asm_emit("_start:\n");
    asm_emit("mov rbx, rsp\n");
    asm_emit("push rbx\n");
}


static void emitExit(backend_ctx_t * ctx)
{
    asm_emit("mov rsp, rbx\n");

    asm_emit("mov rax, 0x3c\n");
    asm_emit("mov rdi, 0x00\n");

    asm_emit("syscall\n");
}


static void makeAssemblyCodeRecursive(backend_ctx_t * ctx, node_t * node)
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
            makeAssemblyCodeRecursive(ctx, node->left);
            makeAssemblyCodeRecursive(ctx, node->right);
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

    asm_emit("push %ld\n", num);
}


static void translatePushVar(backend_ctx_t * ctx, size_t var_index)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // assert(ctx->id_table[var_index].type == VAR);

    name_addr_t * addr = getNameAddr(ctx, var_index);
    if (addr->is_global)
        asm_emit("push QWORD [rbx + (%ld)]", addr->rel_addr);
    else
        asm_emit("push QWORD [rbp + (%ld)]", addr->rel_addr);

    asm_emit("      ; %s\n", ctx->id_table[var_index].name);
}


static void translateAddSubMulDiv(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->left);
    translateExpression(ctx, node->right);

    enum oper op_num = node->val.op;

    if (op_num == DIV)
        asm_emit("xor rdx, rdx\n");

    asm_emit("pop rcx\n");
    asm_emit("pop rax\n");

    switch(op_num){
        case ADD: asm_emit("add rax, rcx\n"); break;
        case SUB: asm_emit("sub rax, rcx\n"); break;
        case MUL: asm_emit("imul rcx\n"); break;
        case DIV: asm_emit("idiv rcx\n"); break;
    }

    asm_emit("push rax\n");
}


static void translateCallHandleArgs(backend_ctx_t * ctx, node_t * arg_node);

static void translateCall(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    node_t * func_node = node->left;
    node_t * arg_tree  = node->right;

    size_t num_of_args = ctx->id_table[func_node->val.id].num_of_args;
    const char * func_name = ctx->id_table[func_node->val.id].name;

    asm_emit("\n; --- CALLING %s ---\n", func_name);

    translateCallHandleArgs(ctx, arg_tree);

    asm_emit("call %s\n", func_name);
    asm_emit("add rsp, %zu\n", 8 * num_of_args);
    asm_emit("push rax\n");

    asm_emit("\n; --- END OF CALLING %s ---\n", func_name);
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

    addNewName(ctx, node->left->val.id, addr, !(ctx->in_function));
    asm_emit("sub rsp, 8        ; %s\n", ctx->id_table[node->left->val.id].name);
}


static void translateAssign(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->right);

    name_addr_t * addr = getNameAddr(ctx, node->left->val.id);
    if (addr->is_global)
        asm_emit("pop QWORD [rbx + (%ld)]\n", addr->rel_addr);
    else
        asm_emit("pop QWORD [rbp + (%ld)]\n", addr->rel_addr);
}


static void translateReturn(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    translateExpression(ctx, node->left);

    asm_emit("pop rax\n");

    asm_emit("mov rsp, rbp\n");
    asm_emit("pop rbp\n");

    asm_emit("ret\n");
}


static void translateIfElse(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // condition expression
    translateExpression(ctx, node->left);

    enterScope(ctx, START_OF_SCOPE);

    asm_emit("pop rsi\n");
    asm_emit("test rsi, rsi\n");

    if (node->right->type == OPR && node->right->val.op == IF_ELSE){
        // we have else
        node_t * if_else_node = node->right;

        asm_emit("jz __IF_%zu_ELSE\n", ctx->if_counter);

        makeAssemblyCodeRecursive(ctx, if_else_node->right);
        asm_emit("jmp __IF_%zu_END\n", ctx->if_counter);

        asm_emit("__IF_%zu_ELSE:\n", ctx->if_counter);
        makeAssemblyCodeRecursive(ctx, if_else_node->left);
    }
    else {
        // we do not have else
        asm_emit("jz __IF_%zu_END\n", ctx->if_counter);
        makeAssemblyCodeRecursive(ctx, node->right);
    }

    asm_emit("__IF_%zu_END:\n", ctx->if_counter);

    leaveScope(ctx, START_OF_SCOPE);
    ctx->if_counter++;
}


static void translateWhile(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    asm_emit("__WHILE_%zu_COND_CHECK:\n", ctx->while_counter);
    // condition expression
    translateExpression(ctx, node->left);

    enterScope(ctx, START_OF_SCOPE);

    asm_emit("pop rsi\n");
    asm_emit("test rsi, rsi\n");
    asm_emit("jz __WHILE_%zu_END\n", ctx->while_counter);

    // body of while
    makeAssemblyCodeRecursive(ctx, node->right);

    asm_emit("jmp __WHILE_%zu_COND_CHECK", ctx->while_counter);
    asm_emit("__WHILE_%zu_END:\n", ctx->while_counter);

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

    asm_emit("jmp __END_OF_%s__\n", func_name);

    asm_emit("%s:\n", func_name);
    asm_emit("mov rbp, rsp\n");
    asm_emit("push rbp\n");

    makeAssemblyCodeRecursive(ctx, func_body);

    asm_emit("__END_OF_%s__:\n", func_name);

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

    // calling standard func
    asm_emit("call __in_standard_func_please_do_not_name_your_funcs_this_name__\n");

    name_addr_t * addr = getNameAddr(ctx, node->left->val.id);

    if (addr->is_global)
        asm_emit("mov [rbx + (%ld)], rax\n", addr->rel_addr);
    else
        asm_emit("mov [rbp + (%ld)], rax\n", addr->rel_addr);
}


static void translateOut(backend_ctx_t * ctx, node_t * node)
{
    logPrint(LOG_DEBUG_PLUS, "%s\n", __PRETTY_FUNCTION__);

    // pushing arg
    translateExpression(ctx, node->left);

    // calling standard func
    asm_emit("call __out_standard_func_please_do_not_name_your_funcs_this_name__\n");
    asm_emit("add rsp, 8\n");
}

