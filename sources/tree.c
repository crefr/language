#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "frontend.h"
#include "tree.h"
#include "logger.h"

void printTreePrefix(fe_context_t * fe, node_t * node)
{
    assert(fe);

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
        printf("%s", fe->ids[node->val.id].name);
        return;
    }

    oper_t oper = opers[node->val.op];

    printf("(%s", oper.name);

    printf("(");
    printTreePrefix(fe, node->left);
    printf(")");

    if (oper.binary){
        printf("(");
        printTreePrefix(fe, node->right);
        printf(")");
    }
    printf(")");
}

void treeDumpGraph(fe_context_t * fe, node_t * root_node)
{
    assert(root_node);

    const int  IMG_WIDTH_IN_PERCENTS = 95;
    const int IMG_HEIGTH_IN_PERCENTS = 70;

    static size_t dump_count = 0;

    const size_t MAX_FILE_NAME = 256;
    char dot_file_name[MAX_FILE_NAME] = "";
    char img_file_name[MAX_FILE_NAME] = "";

    system("mkdir -p logs/dots/");
    system("mkdir -p logs/imgs/");

    sprintf(dot_file_name, "logs/dots/graph_%zu.dot", dump_count);
    sprintf(img_file_name, "logs/imgs/graph_%zu.svg", dump_count);

    FILE * dot_file = fopen(dot_file_name, "w");
    treeMakeDot(fe, root_node, dot_file);
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

static void nodeMakeDot(fe_context_t * fe, FILE * dot_file, node_t * node, node_t * parent);

void treeMakeDot(fe_context_t * fe, node_t * node, FILE * dot_file)
{
    assert(node);
    assert(dot_file);

    fprintf(dot_file, "digraph {\n");
    fprintf(dot_file, "node [style=filled,color=\"#000000\"]\n");

    nodeMakeDot(fe, dot_file, node, NULL);

    fprintf(dot_file, "}\n");
}

static void dotPrintNode(fe_context_t * fe, FILE * dot_file, node_t * node);

static void nodeMakeDot(fe_context_t * fe, FILE * dot_file, node_t * node, node_t * parent)
{
    assert(node);
    assert(dot_file);

    size_t node_num = (size_t)node;

    dotPrintNode(fe, dot_file, node);

    if (parent != NULL){
        size_t node_parent_num = (size_t)(parent);

        if (node == parent->left)
            fprintf(dot_file, "node_%zu:f0->node_%zu;\n", node_parent_num, node_num);
        else
            fprintf(dot_file, "node_%zu:f1->node_%zu;\n", node_parent_num, node_num);
    }

    if (node->left  != NULL)
        nodeMakeDot(fe, dot_file, node->left, node);

    if (node->right != NULL)
        nodeMakeDot(fe, dot_file, node->right, node);
}

const uint32_t IDR_COLOR = 0xFFAAAAFF;
const uint32_t NUM_COLOR = 0xAAAAFFFF;
const uint32_t OPR_COLOR = 0xAAFFAAFF;

static void dotPrintNode(fe_context_t * fe, FILE * dot_file, node_t * node)
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
        sprintf(elem_str, "type = OPR, val = '%s'", opers[node->val.op].name);
        color_to_dump = OPR_COLOR;
    }
    else if (node->type == IDR){
        sprintf(elem_str, "type = IDR, val = '%s'", fe->ids[node->val.op].name);
        color_to_dump = IDR_COLOR;
    }

    fprintf(dot_file, "node_%zu"
                "[shape=Mrecord,label="
                "\"{node at %p | \\\" %s \\\" | {<f0> left = %p |<f1> right = %p}}\","
                "fillcolor=\"#%08X\"];\n",
                node_num, node, elem_str, node->left, node->right, color_to_dump);
}
