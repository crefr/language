#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "tree.h"
#include "IR_handler.h"
#include "middleend.h"
#include "logger.h"

static double calcOper(enum oper op_num, double left_val, double right_val);

static node_t * delNeutralInCommutatives(me_context_t * me, node_t * node, bool * changed_tree);

static node_t * delNeutralInNonCommutatives(me_context_t * me, node_t * node, bool * changed_tree);

me_context_t middleendInit(const char * tree_file_name)
{
    me_context_t context = {};

    context.nodes = (node_t *)calloc(MAX_NODES_NUM, sizeof(*context.nodes));

    tree_context_t tree = {};
    tree.cur_node = context.nodes;
    tree.id_size = 0;
    tree.ids = context.ids;

    context.root = readTreeFromIR(&tree, tree_file_name);
    context.id_size = tree.id_size;

    context.free_node = tree.cur_node + 1;

    return context;
}

void middleendRun(const char * tree_file_name)
{
    me_context_t context = middleendInit(tree_file_name);

    context.root = simplifyExpression(&context, context.root);

    FILE * tree_file = fopen(tree_file_name, "w");

    tree_context_t ir_context = {};
    ir_context.cur_node = context.root;
    ir_context.id_size  = context.id_size;
    ir_context.ids      = context.ids;

    writeTreeToFile(&ir_context, context.root, tree_file);

    fclose(tree_file);

    middleendDestroy(&context);
}

node_t * newNode(me_context_t * context, enum elem_type type, union value val, node_t * left, node_t * right)
{
    node_t * node = context->free_node;
    context->free_node++;

    node->type = type;
    node->val = val;

    node->left = left;
    node->right = right;

    return node;
}

node_t * newOprNode(me_context_t * context, enum oper op_num, node_t * left, node_t * right)
{
    union value val = {};
    val.op = op_num;

    return newNode(context, OPR, val, left, right);
}

node_t * newNumNode(me_context_t * context, double number)
{
    union value val = {};
    val.number = number;

    return newNode(context, NUM, val, NULL, NULL);
}

void middleendDestroy(me_context_t * me)
{
    free(me->nodes);

    me->nodes = NULL;
}

node_t * simplifyExpression(me_context_t * me, node_t * node)
{
    assert(node);

    bool changing = true;
    while (changing){
        changing = false;

        node = foldConstants(me, node, &changing);
        node = deleteNeutral(me, node, &changing);
    }

    return node;
}

node_t * foldConstants(me_context_t * me, node_t * node, bool * changed_tree)
{
    if (node == NULL)
        return NULL;

    if (node->type == VAR)
        return node;

    if (node->type == NUM)
        return node;

    enum oper op_num = node->val.op;

    node->left  = foldConstants(me, node->left , changed_tree);
    node->right = foldConstants(me, node->right, changed_tree);

    if (! opers[op_num].can_simple)
        return node;

    double new_val = 0.;

    if (opers[op_num].binary){
        if (! node->left)
            return node;
        if (! node->right)
            return node;

        if (node->left->type == NUM && node->right->type == NUM){
            double  left_val = node->left ->val.number;
            double right_val = node->right->val.number;

            new_val = calcOper(op_num, left_val, right_val);
        }
        else
            return node;
    }
    else {
        if (! node->left)
            return node;

        if (node->left->type == NUM){
            double val = node->left->val.number;
            new_val = calcOper(op_num, val, 0);
        }
        else
            return node;
    }

    node_t * new_node = newNumNode(me, new_val);

    *changed_tree = true;

    return new_node;
}

static double calcOper(enum oper op_num, double left_val, double right_val)
{
    double new_val = 0.;

    switch (op_num){
        case ADD:
            new_val = left_val + right_val;
            break;

        case SUB:
            new_val = left_val - right_val;
            break;

        case MUL:
            new_val = left_val * right_val;
            break;

        case DIV:
            new_val = left_val / right_val;
            break;

        case POW:
            new_val = pow(left_val, right_val);
            break;

        case SIN:
            new_val = sin(left_val);
            break;

        case COS:
            new_val = cos(left_val);
            break;

        case TAN:
            new_val = tan(left_val);
            break;

        case LN:
            new_val = log(left_val);
            break;

        case LOG:
            new_val = log(right_val) / log(left_val);
            break;

        default:
            fprintf(stderr, "CANNOT CALCULATE THIS OPERATION: %d\n", op_num);
            exit(1);
    }
    return new_val;
}


node_t * deleteNeutral(me_context_t * me, node_t * node, bool * changed_tree)
{
    if (node == NULL)
        return NULL;

    if (node->type != OPR)
        return node;

    node->left  = deleteNeutral(me, node->left,  changed_tree);
    node->right = deleteNeutral(me, node->right, changed_tree);

    if (opers[node->val.op].commutative)
        return delNeutralInCommutatives(me, node, changed_tree);

    return delNeutralInNonCommutatives(me, node, changed_tree);
}

static node_t * delNeutralInCommutatives(me_context_t * me, node_t * node, bool * changed_tree)
{
    assert(node);

    bool last_changed = *changed_tree;
    *changed_tree = true;

    node_t * cur_node = node->left;
    node_t * another_node = node->right;
    while (cur_node != NULL) {
        if (cur_node->type == NUM){
            switch (node->val.op){
                case MUL:
                    logPrint(LOG_DEBUG_PLUS, "in mul case...\n");
                    /* x*1 = x */
                    if (cur_node->val.number == 1.){
                        return another_node;
                    }
                    /* x*0 = 0 */
                    else if (cur_node->val.number == 0.){
                        return cur_node;
                    }
                    break;
                case ADD:
                    logPrint(LOG_DEBUG_PLUS, "in add case...\n");
                    if (cur_node->val.number == 0.){
                        return another_node;
                    }
                    break;
                default:
                    break;
            }
        }

        if (cur_node == node->left){
            cur_node     = node->right;
            another_node = node->left;
        }
        else
            cur_node = NULL;
    }

    *changed_tree = last_changed;

    return node;
}

static node_t * delNeutralInNonCommutatives(me_context_t * me, node_t * node, bool * changed_tree)
{
    assert(node);

    node_t * left  = node->left;
    node_t * right = node->right;

    bool last_changed = *changed_tree;
    *changed_tree = true;

    switch (node->val.op){
        case DIV:
            if (right->type == NUM){
                if (right->val.number == 1.){
                    return left;
                }
            }
            break;

        case SUB:
            if (right->type == NUM){
                if (right->val.number == 0.){
                    return left;
                }
            }
            break;
        case POW:
            if (right->type == NUM){
                if (right->val.number == 1.){
                    return left;
                }
                else if (right->val.number == 0.){
                    node_t * new_node = newNumNode(me, 1.);
                    return new_node;
                }
            }
            if (left->type == NUM){
                if (left->val.number == 1. || left->val.number == 0.){
                    return left;
                }
            }
            break;
    }

    *changed_tree = last_changed;

    return node;
}
