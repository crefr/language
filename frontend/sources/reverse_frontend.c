#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tree.h"
#include "logger.h"
#include "reverse_frontend.h"

static void printCodeFromTreeRecursive(FILE * out_file, tree_context_t * context, node_t * node);

void printCodeFromTree(const char * out_file_name, tree_context_t * context, node_t * root)
{
    assert(out_file_name);
    assert(root);
    assert(context);

    FILE * out_file = fopen(out_file_name, "w");

    fprintf(out_file, "#AUTOGENERATED CODE#\n");
    printCodeFromTreeRecursive(out_file, context, root);

    fclose(out_file);
}

static void printCodeFromTreeRecursive(FILE * out_file, tree_context_t * context, node_t * node)
{
    assert(out_file);
    assert(context);

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

    enum oper op_num = node->val.op;
    switch(op_num){
        case SEP:
            printCodeFromTreeRecursive(out_file, context, node->left);
            fprintf(out_file, ";\n");
            printCodeFromTreeRecursive(out_file, context, node->right);

            break;

        case IN: case OUT: case COS: case SIN: case LN: case TAN:
            fprintf(out_file, "%s(", opers[op_num].name);
            printCodeFromTreeRecursive(out_file, context, node->left);
            fprintf(out_file, ")");

            break;

        case ASSIGN:
            printCodeFromTreeRecursive(out_file, context, node->left);
            fprintf(out_file, "%s", opers[op_num].name);
            printCodeFromTreeRecursive(out_file, context, node->right);

            break;

        case ADD: case SUB: case MUL: case DIV:
            // TODO: decrease number of brackets
            fprintf(out_file, "(");
            printCodeFromTreeRecursive(out_file, context, node->left);

            fprintf(out_file, "%s", opers[op_num].name);

            printCodeFromTreeRecursive(out_file, context, node->right);
            fprintf(out_file, ")");

            break;

        case IF: case WHILE:
            fprintf(out_file, "%s (", opers[op_num].name);
            printCodeFromTreeRecursive(out_file, context, node->left);
            fprintf(out_file, ")\n");

            fprintf(out_file, "%s\n", opers[BEGIN].name);
            printCodeFromTreeRecursive(out_file, context, node->right);
            fprintf(out_file, "%s", opers[ENDING].name);

            break;

        default:
            fprintf(stderr, "cannot generate it\n");
            break;
    }
}