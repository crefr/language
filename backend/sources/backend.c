#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "logger.h"
#include "backend.h"
#include "IR_handler.h"

static void readTreeForBackend(be_context_t * be, const char * tree_file_name);

static void makeAssemblyCodeRecursive(be_context_t * be, node_t * cur_node);

static void translateExpression(be_context_t * be, node_t * cur_node);

be_context_t backendInit(size_t nodes_num, const char * asm_file_name, const char * tree_file_name)
{
    assert(asm_file_name);
    assert(tree_file_name);

    be_context_t context = {};

    context.asm_file = fopen(asm_file_name, "w");
    assert(context.asm_file);   // TODO: make if-statement

    context.nodes = (node_t *)calloc(nodes_num, sizeof(node_t));
    context.tree_size = nodes_num;

    context.cur_node = context.nodes;
    context.id_size = 0;

    readTreeForBackend(&context, tree_file_name);

    context.root = context.nodes;

    return context;
}

static void readTreeForBackend(be_context_t * be, const char * tree_file_name)
{
    assert(be);
    assert(tree_file_name);

    tree_context_t sub_context = {};

    sub_context.cur_node = be->cur_node;
    sub_context.id_size  = be->id_size;
    sub_context.ids      = be->ids;

    readTreeFromIR(&sub_context, tree_file_name);

    be->cur_node = sub_context.cur_node;
    be->id_size  = sub_context.id_size;
}

void backendDtor(be_context_t * context)
{
    assert(context);

    free(context->nodes);

    context->nodes    = NULL;
    context->cur_node = NULL;

    fclose(context->asm_file);
}

/*---------------------------TRANSLATING TO ASM---------------------------*/

void makeAssemblyCode(be_context_t * be)
{
    assert(be);

    logPrint(LOG_DEBUG, "started translating to asm...\n");

    makeAssemblyCodeRecursive(be, be->root);
    fprintf(be->asm_file, "HLT\n");

    logPrint(LOG_DEBUG, "successfully translated to asm!\n");
}

#define asmPrintf(...) fprintf(be->asm_file, __VA_ARGS__)

// TODO: make memory indexes independent from indexes in name table
#define asmPrintVAR(var) asmPrintf(" [%u]     ; %s\n", var, be->ids[var].name)

static void makeAssemblyCodeRecursive(be_context_t * be, node_t * cur_node)
{
    assert(be);

    static size_t if_counter = 0;
    static size_t while_counter = 0;

    if (cur_node == NULL)
        return;

    assert(cur_node->type == OPR);

    switch(cur_node->val.op){
        case SEP:
            makeAssemblyCodeRecursive(be, cur_node->left);
            makeAssemblyCodeRecursive(be, cur_node->right);

            break;

        case ASSIGN:
            translateExpression(be, cur_node->right);

            asmPrintf("POP ");
            asmPrintVAR(cur_node->left->val.id);

            break;

        case IN:
            asmPrintf("IN\n");

            asmPrintf("POP ");
            asmPrintVAR(cur_node->left->val.id);

            break;

        case OUT:
            translateExpression(be, cur_node->left);
            asmPrintf("OUT\n");

            break;

        case IF: {
            size_t if_index = if_counter;
            if_counter++;

            translateExpression(be, cur_node->left);

            asmPrintf("PUSH 0\n");
            asmPrintf("JE IF_END_%zu:\n", if_index);
            makeAssemblyCodeRecursive(be, cur_node->right);
            asmPrintf("IF_END_%zu:\n", if_index);

            if_counter++;
            break;
        }

        case WHILE: {
            size_t while_index = while_counter;
            while_counter++;

            asmPrintf("WHILE_BEGIN_%zu:\n", while_index);

            translateExpression(be, cur_node->left);
            asmPrintf("PUSH 0\n");
            asmPrintf("JE WHILE_END_%zu:\n", while_index);

            makeAssemblyCodeRecursive(be, cur_node->right);

            asmPrintf("JMP WHILE_BEGIN_%zu:\n", while_index);
            asmPrintf("WHILE_END_%zu:\n", while_index);

            break;
        }

        case FUNC_DECL: {
            const char * func_name = be->ids[cur_node->left->val.id].name;
            asmPrintf("JMP END_OF_FUNC_%s: ;skipping func body\n", func_name);

            asmPrintf("%s:\n", func_name);

            makeAssemblyCodeRecursive(be, cur_node->right);

            asmPrintf("END_OF_FUNC_%s:\n", func_name);

            break;
        }

        case RETURN: {
            translateExpression(be, cur_node->left);

            asmPrintf("POP  RAX\n");
            asmPrintf("RET\n");

            break;
        }


        default:
            fprintf(stderr, "ERROR: failed to translate to asm (do not know this operator)\n");
            return;
    }
}

static void translateExpression(be_context_t * be, node_t * cur_node)
{
    if (cur_node->type == NUM){
        asmPrintf("PUSH %lg\n", cur_node->val.number);
        return;
    }

    if (cur_node->type == IDR){
        // TODO: make memory indexes independent from indexes in name table
        asmPrintf("PUSH");
        asmPrintVAR(cur_node->val.id);
        return;
    }

    // if (...type == OPR)

    enum oper op_num = cur_node->val.op;
    if (opers[op_num].binary){
        translateExpression(be, cur_node->left);
        translateExpression(be, cur_node->right);
    }
    else
        translateExpression(be, cur_node->left);

    asmPrintf("%s\n", opers[op_num].asm_str);
}

#undef asmPrintVAR
#undef asmPrintf
