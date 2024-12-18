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

    fprintf(be->asm_file, "; AUTOGENERATED CODE\n");

    makeAssemblyCodeRecursive(be, be->root);

    fprintf(be->asm_file, "HLT\n");

    logPrint(LOG_DEBUG, "successfully translated to asm!\n");
}

#define asmPrintf(...) fprintf(be->asm_file, __VA_ARGS__)

static void asmPrintVAR(be_context_t * be, size_t var_index)
{
    assert(be);

    if (! be->ids[var_index].is_local) {
        asmPrintf(" [%u]     ; %s\n", be->ids[var_index].address, be->ids[var_index].name);
        return;
    }

    asmPrintf(" [RBX %zu] ; %s (local)\n", be->ids[var_index].address, be->ids[var_index].name);
}

static void makeAssemblyCodeRecursive(be_context_t * be, node_t * cur_node)
{
    assert(be);

    /*-----static variables used to generate asm code properly-----*/
    static size_t if_counter = 0;
    static size_t while_counter = 0;

    static size_t global_var_counter = 0;
    static size_t  local_var_counter = 0;

    static bool in_function = false;
    /*-------------------------------------------------------------*/

    if (cur_node == NULL)
        return;

    assert(cur_node->type == OPR);

    switch(cur_node->val.op){
        case SEP:
            makeAssemblyCodeRecursive(be, cur_node->left);
            makeAssemblyCodeRecursive(be, cur_node->right);

            break;

        case ASSIGN: {
            translateExpression(be, cur_node->right);

            asmPrintf("POP ");
            size_t id_index = cur_node->left->val.id;

            asmPrintVAR(be, id_index);

            break;
        }

        case VAR_DECL: {
            node_t * var_node = cur_node->left;
            size_t id_index = var_node->val.id;

            if (in_function){
                be->ids[id_index].is_local = true;
                be->ids[id_index].address = local_var_counter;

                local_var_counter++;
            }
            else {
                be->ids[id_index].is_local = false;
                be->ids[id_index].address = global_var_counter;

                global_var_counter++;
            }

            break;
        }

        case IN:
            asmPrintf("IN\n");

            asmPrintf("POP ");
            asmPrintVAR(be, cur_node->left->val.id);

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
            local_var_counter = 0;
            in_function = true;

            // making arguments local and giving them addresses
            node_t * cur_arg_node = cur_node->left->right;

            // args adresses are RBX+0, RBX+1, RBX+2, etc.
            size_t arg_index = 0;

            while (cur_arg_node != NULL){
                size_t cur_id = cur_arg_node->left->val.id;

                be->ids[cur_id].is_local = true;
                be->ids[cur_id].address  = arg_index;

                arg_index++;
                cur_arg_node = cur_arg_node->right;
            }

            // args are local vars too
            local_var_counter += arg_index;

            // taking func_name from left node of FUNC_HEADER node
            const char * func_name = be->ids[cur_node->left->left->val.id].name;

            asmPrintf("JMP END_OF_FUNC_%s: ;skipping func body\n", func_name);

            asmPrintf("%s:\n", func_name);

            // body of the function

            makeAssemblyCodeRecursive(be, cur_node->right);

            asmPrintf("END_OF_FUNC_%s:\n", func_name);

            in_function = false;

            break;
        }

        case RETURN: {
            translateExpression(be, cur_node->left);

            asmPrintf("POP  RAX\n");

            asmPrintf("RET\n");

            break;
        }

        case CALL: {
            const char * func_name = be->ids[cur_node->left->val.id].name;

            asmPrintf("; call\n");

            // giving arguments to the function
            node_t * cur_arg_node = cur_node->right;

            // args adresses are RBX+0, RBX+1, RBX+2, etc.
            size_t arg_index = 0;

            asmPrintf("; giving args\n");
            while (cur_arg_node != NULL){
                translateExpression(be, cur_arg_node->left);

                // we need to add ...var_counter because we have not yet shifted base pointer (RBX)
                if (in_function)
                    asmPrintf("POP  [RBX %zu]\n", arg_index + local_var_counter);
                else
                    asmPrintf("POP  [%zu]\n", arg_index + global_var_counter);

                arg_index++;
                cur_arg_node = cur_arg_node->right;
            }
            asmPrintf("; ended giving args\n");

            // setting base pointer (RBX)

            // old base pointer
            asmPrintf("; pushing old base pointer (RBX)\n");
            asmPrintf("PUSH RBX\n");

            asmPrintf("; shifting base pointer (RBX)\n");
            if (in_function)
                asmPrintf("PUSH RBX %zu\n", local_var_counter);
            else
                asmPrintf("PUSH %zu\n", global_var_counter);
            asmPrintf("POP  RBX\n");

            asmPrintf("; ended shifting base pointer (RBX)\n");

            asmPrintf("CALL %s:\n", func_name);

            // returning old base pointer
            asmPrintf("POP  RBX\n");

            asmPrintf("PUSH RAX\n");

            asmPrintf("; call ended\n");


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
        size_t id_index = cur_node->val.id;

        asmPrintf("PUSH");
        asmPrintVAR(be, id_index);

        return;
    }

    // if (...type == OPR)

    if (cur_node->val.op == CALL){
        makeAssemblyCodeRecursive(be, cur_node);
        return;
    }

    enum oper op_num = cur_node->val.op;
    if (opers[op_num].binary){
        translateExpression(be, cur_node->left);
        translateExpression(be, cur_node->right);
    }
    else
        translateExpression(be, cur_node->left);

    asmPrintf("%s\n", opers[op_num].asm_str);
}

#undef asmPrintf
