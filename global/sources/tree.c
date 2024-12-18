#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "tree.h"
#include "logger.h"

void printTreePrefix(tree_context_t * tree, node_t * node)
{
    assert(tree);

    if (node == NULL)
        return;

    if (node->type == END){
        printf("$\n");
        return;
    }

    if (node->type == NUM){
        printf("%lg", node->val.number);
        return;
    }

    if (node->type == IDR){
        printf("%s", tree->ids[node->val.id].name);
        return;
    }

    oper_t oper = opers[node->val.op];

    printf("(%s", oper.name);

    printf("(");
    printTreePrefix(tree, node->left);
    printf(")");

    if (oper.binary){
        printf("(");
        printTreePrefix(tree, node->right);
        printf(")");
    }
    printf(")");
}

void treeDumpGraph(tree_context_t * tree, node_t * root_node, const char * log_folder)
{
    assert(root_node);

    const int  IMG_WIDTH_IN_PERCENTS = 95;
    const int IMG_HEIGTH_IN_PERCENTS = 70;

    static size_t dump_count = 0;

    const size_t MAX_FILE_NAME = 256;
    char dot_file_name[MAX_FILE_NAME] = "";
    char img_file_name[MAX_FILE_NAME] = "";

    char mkdir_dots_cmd[MAX_FILE_NAME] = "";
    char mkdir_imgs_cmd[MAX_FILE_NAME] = "";

    sprintf(mkdir_dots_cmd, "mkdir -p %s/dots/", log_folder);
    sprintf(mkdir_imgs_cmd, "mkdir -p %s/imgs/", log_folder);

    system(mkdir_dots_cmd);
    system(mkdir_imgs_cmd);

    sprintf(dot_file_name, "%s/dots/graph_%zu.dot", log_folder, dump_count);
    sprintf(img_file_name, "%s/imgs/graph_%zu.svg", log_folder, dump_count);

    FILE * dot_file = fopen(dot_file_name, "w");
    treeMakeDot(tree, root_node, dot_file);
    fclose(dot_file);

    char sys_dot_cmd[MAX_FILE_NAME] = "";
    sprintf(sys_dot_cmd, "dot %s -Tsvg -o %s", dot_file_name, img_file_name);
    system(sys_dot_cmd);

    char img_file_name_log[MAX_FILE_NAME] = "";
    sprintf(img_file_name_log, "imgs/graph_%zu.svg", dump_count);
    logPrint(LOG_DEBUG, "<img src = %s width = \"%d%%\" height = \"%d%%\">",
                        img_file_name_log,
                        IMG_WIDTH_IN_PERCENTS,
                        IMG_HEIGTH_IN_PERCENTS);

    logPrint(LOG_DEBUG, "<hr>");

    dump_count++;
}

static void nodeMakeDot(tree_context_t * tree, FILE * dot_file, node_t * node, node_t * parent);

void treeMakeDot(tree_context_t * tree, node_t * node, FILE * dot_file)
{
    assert(node);
    assert(dot_file);

    fprintf(dot_file, "digraph {\n");
    fprintf(dot_file, "node [style=filled,color=\"#000000\"]\n");

    nodeMakeDot(tree, dot_file, node, NULL);

    fprintf(dot_file, "}\n");
}

static void dotPrintNode(tree_context_t * tree, FILE * dot_file, node_t * node);

static void nodeMakeDot(tree_context_t * tree, FILE * dot_file, node_t * node, node_t * parent)
{
    assert(node);
    assert(dot_file);

    size_t node_num = (size_t)node;

    dotPrintNode(tree, dot_file, node);

    if (parent != NULL){
        size_t node_parent_num = (size_t)(parent);

        if (node == parent->left)
            fprintf(dot_file, "node_%zu:f0->node_%zu;\n", node_parent_num, node_num);
        else
            fprintf(dot_file, "node_%zu:f1->node_%zu;\n", node_parent_num, node_num);
    }

    if (node->left  != NULL)
        nodeMakeDot(tree, dot_file, node->left, node);

    if (node->right != NULL)
        nodeMakeDot(tree, dot_file, node->right, node);
}

const uint32_t IDR_COLOR = 0xFFAAAAFF;
const uint32_t NUM_COLOR = 0xAAAAFFFF;
const uint32_t OPR_COLOR = 0xAAFFAAFF;

const uint32_t FIC_COLOR = 0xDDDDDDFF;

static void dotPrintNode(tree_context_t * tree, FILE * dot_file, node_t * node)
{
    size_t node_num = (size_t)node;

    const size_t MAX_ELEM_STR_LEN = 128;
    char elem_str[MAX_ELEM_STR_LEN] = "";

    uint32_t color_to_dump = 0;

    if (node->type == NUM){
        sprintf(elem_str, "type = NUM, val = %lg", node->val.number);
        color_to_dump = NUM_COLOR;
    }
    else if (node->type == OPR){
        const char * name = NULL;

        if (opers[node->val.op].name != NULL)
            name = opers[node->val.op].name;
        else {
            switch (node->val.op){
                case CALL:
                    name = "_CALL_";
                    break;

                case FUNC_HEADER:
                    name = "__FUNC_HEADER__";
                    break;

                default:
                    break; // literally do not know what else to do here...
            }
        }
        sprintf(elem_str, "type = OPR, val = '%s'", name);

        // we need fictive operators to be another color
        if (node->val.op == SEP || node->val.op == ARG_SEP)
            color_to_dump = FIC_COLOR;
        else
            color_to_dump = OPR_COLOR;
    }
    else if (node->type == IDR){
        const char * type_str = (tree->ids[node->val.id].type == VAR) ? "VAR" : "FUNC";

        sprintf(elem_str, "type = IDR, val = '%s', IDR_type = %s", tree->ids[node->val.op].name, type_str);
        color_to_dump = IDR_COLOR;
    }

    fprintf(dot_file, "node_%zu"
                "[shape=Mrecord,label="
                "\"{node at %p | \\\" %s \\\" | {<f0> left = %p |<f1> right = %p}}\","
                "fillcolor=\"#%08X\"];\n",
                node_num, node, elem_str, node->left, node->right, color_to_dump);
}
