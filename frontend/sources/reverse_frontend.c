#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "tree.h"
#include "logger.h"
#include "reverse_frontend.h"

static void printCodeFromTreeRecursive(FILE * out_file, tree_context_t * context, node_t * node, bool need_tabs);

static void printTabs(FILE * out_file, size_t tabs_num);

static void printFuncArgs(FILE * out_file, tree_context_t * context, node_t * start_node);

void printCodeFromTree(const char * out_file_name, tree_context_t * context, node_t * root)
{
    assert(out_file_name);
    assert(root);
    assert(context);

    FILE * out_file = fopen(out_file_name, "w");

    fprintf(out_file, "#AUTOGENERATED CODE#\n");
    printCodeFromTreeRecursive(out_file, context, root, true);

    fclose(out_file);
}

static void printCodeFromTreeRecursive(FILE * out_file, tree_context_t * context, node_t * node, bool need_tabs)
{
    assert(out_file);
    assert(context);

    static size_t tab_num = 0;

    if (node == NULL)
        return;

    if (node->type == NUM){
        fprintf(out_file, "%lg", node->val.number);
        return;
    }

    if (node->type == IDR){
        fprintf(out_file, "%s", context->ids[node->val.id].name);
        return;
    }

    // if (...type == OPR)

    printf("tabs: %d\n", tab_num);

    if (need_tabs)
        printTabs(out_file, tab_num);

    enum oper op_num = node->val.op;
    switch(op_num){
        case SEP:
            printCodeFromTreeRecursive(out_file, context, node->left, false);
            fprintf(out_file, ";\n");

            printCodeFromTreeRecursive(out_file, context, node->right, true);

            break;

        case IN: case OUT: case COS: case SIN: case LN: case TAN:
            fprintf(out_file, "%s(", opers[op_num].name);
            printCodeFromTreeRecursive(out_file, context, node->left, false);
            fprintf(out_file, ")");

            break;

        case ASSIGN:
            printCodeFromTreeRecursive(out_file, context, node->left, false);
            fprintf(out_file, "%s", opers[op_num].name);
            printCodeFromTreeRecursive(out_file, context, node->right, false);

            break;

        case ADD: case SUB: case MUL: case DIV:
            // TODO: decrease number of brackets
            fprintf(out_file, "(");
            printCodeFromTreeRecursive(out_file, context, node->left, false);

            fprintf(out_file, "%s", opers[op_num].name);

            printCodeFromTreeRecursive(out_file, context, node->right, false);
            fprintf(out_file, ")");

            break;

        case IF: case WHILE:
            fprintf(out_file, "%s (", opers[op_num].name);
            printCodeFromTreeRecursive(out_file, context, node->left, false);
            fprintf(out_file, ")\n");

            printTabs(out_file, tab_num);

            fprintf(out_file, "%s\n", opers[BEGIN].name);
            tab_num++;

            printCodeFromTreeRecursive(out_file, context, node->right, true);

            tab_num--;
            printTabs(out_file, tab_num);
            fprintf(out_file, "%s", opers[ENDING].name);

            break;

        case CALL: {
            // getting function name (left node)
            const char * func_name = context->ids[node->left->val.id].name;

            fprintf(out_file, "%s(", func_name);

            // on the right of the CALL we have arg list (if args presented)
            printFuncArgs(out_file, context, node->right);

            fprintf(out_file, ")");

            break;
        }

        case FUNC_DECL: {
            // FUNC_HEADER node on the left
            node_t * func_header = node->left;

            const char * func_name = context->ids[func_header->left->val.id].name;

            fprintf(out_file, "%s %s(", opers[FUNC_DECL].name, func_name);

            printFuncArgs(out_file, context, func_header->right);

            fprintf(out_file, ")\n");

            fprintf(out_file, "%s\n", opers[BEGIN].name);
            tab_num++;

            // function body tree is on the left
            printCodeFromTreeRecursive(out_file, context, node->right, true);

            tab_num--;
            fprintf(out_file, "%s", opers[ENDING].name);

            break;
        }

        case VAR_DECL: {
            const char * var_name = context->ids[node->left->val.id].name;

            fprintf(out_file, "%s %s", opers[VAR_DECL].name, var_name);

            break;
        }

        case RETURN:
            fprintf(out_file, "return ");
            printCodeFromTreeRecursive(out_file, context, node->left, false);

            break;

        default:
            fprintf(stderr, "cannot generate it (%d)\n", node->val.op);
            break;
    }
}

static void printFuncArgs(FILE * out_file, tree_context_t * context, node_t * start_node)
{
    assert(out_file);
    assert(context);

    if (start_node == NULL)
        return;

    node_t * arg_node = start_node;

    // the first arg is unique because id do not have comma before it
    printCodeFromTreeRecursive(out_file, context, arg_node->left, false);

    arg_node = arg_node->right;

    // other args if presented
    while (arg_node != NULL){
        fprintf(out_file, ", ");
        printCodeFromTreeRecursive(out_file, context, arg_node->left, false);

        arg_node = arg_node->right;
    }
}

static void printTabs(FILE * out_file, size_t tabs_num)
{
    assert(out_file);

    for (size_t tab_index = 0; tab_index < tabs_num; tab_index++){
        fputc('\t', out_file);
    }
}
