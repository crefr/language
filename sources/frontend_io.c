#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "frontend.h"
#include "logger.h"

static void writeNameTable(fe_context_t * fe, FILE * out_file);

static void writeTreeToFileRecursive(fe_context_t * fe, node_t * node, FILE * out_file, const size_t tab_num);

char * readProgramText(const char * file_name)
{
    assert(file_name);

    logPrint(LOG_DEBUG, "reading program text\n");

    struct stat st = {};
    stat(file_name, &st);

    size_t file_len = st.st_size;
    logPrint(LOG_DEBUG, "file size: %zu\n", file_len);

    FILE * file = fopen(file_name, "r");
    assert(file);

    char * code_str = (char *)calloc(file_len, *code_str);
    fread(code_str, sizeof(*code_str), file_len, file);

    fclose(file);

    return code_str;
}

const char * const SIGN_STRING = "TXT:1";

void writeTreeToFile(fe_context_t * fe, node_t * root, FILE * out_file)
{
    assert(fe);
    assert(root);
    assert(out_file);

    fprintf(out_file, SIGN_STRING);
    fprintf(out_file, "\n");

    writeNameTable(fe, out_file);

    writeTreeToFileRecursive(fe, root, out_file, 0);

    fprintf(out_file, "\n");
}

static void writeNameTable(fe_context_t * fe, FILE * out_file)
{
    assert(fe);
    assert(out_file);

    fprintf(out_file, "nametable {\n");

    for (size_t id_index = 0; id_index < fe->id_size; id_index++){
        fprintf(out_file, "\t%04zu: \"%s\";\n", id_index, fe->ids[id_index].name);
    }

    fprintf(out_file, "}\n");
}

static void writeTreeToFileRecursive(fe_context_t * fe, node_t * node, FILE * out_file, const size_t tab_num)
{
    assert(fe);
    assert(out_file);

    if (node == NULL){
        fprintf(out_file, "{}");
        return;
    }

    if (node->type == NUM){
        fprintf(out_file, "{NUM:%lg}", node->val.number);
        return;
    }

    if (node->type == IDR){
        fprintf(out_file, "{IDR:%u}", node->val.id);
        return;
    }

    oper_t oper = opers[node->val.op];

    fprintf(out_file, "{OPR:%d\n", node->val.op);

    writeTreeToFileRecursive(fe, node->left , out_file, tab_num + 1);
    writeTreeToFileRecursive(fe, node->right, out_file, tab_num + 1);

    fprintf(out_file, "}\n");
}
