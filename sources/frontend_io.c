#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "frontend.h"
#include "logger.h"

const char * const SIGN_STRING  = "TXT:1";
const size_t SIGN_MAX_LEN = 32;
const size_t BUFFER_LEN   = 32;

static void writeNameTable(fe_context_t * fe, FILE * out_file);

static void writeTreeToFileRecursive(fe_context_t * fe, node_t * node, FILE * out_file, const size_t tab_num);

static size_t getFileSize(const char * file_name);

static int readSignature(fe_context_t * fe, const char ** cur_pos);

static node_t * readTreeFromIRrecursive(fe_context_t * fe, const char ** cur_pos);

static int readNameTable(fe_context_t * fe, const char ** cur_pos);

char * readProgramText(const char * file_name)
{
    assert(file_name);

    logPrint(LOG_DEBUG, "reading program text\n");

    FILE * file = fopen(file_name, "r");
    assert(file);

    size_t file_len = getFileSize(file_name) + 1;

    logPrint(LOG_DEBUG, "file size: %zu\n", file_len);

    char * code_str = (char *)calloc(file_len, sizeof(*code_str));
    fread(code_str, sizeof(*code_str), file_len, file);

    fclose(file);

    return code_str;
}

node_t * readTreeFromIR(fe_context_t * fe, const char * file_name)
{
    assert(file_name);
    assert(fe);

    logPrint(LOG_DEBUG, "reading tree from IR\n");

    size_t file_len = getFileSize(file_name) + 1;

    FILE * file = fopen(file_name, "r");

    char * buffer = (char *)calloc(file_len, sizeof(*buffer));
    fread(buffer, sizeof(*buffer), file_len, file);

    fclose(file);

    const char * cur_pos = buffer;

    // checking signature
    if (! readSignature(fe, &cur_pos)){
        logPrint(LOG_RELEASE, "ERROR: invalid signature\n");
        fprintf(stderr, "ERROR: invalid signature\n");

        return NULL;
    }

    // reading name table
    if (! readNameTable(fe, &cur_pos)){
        logPrint(LOG_RELEASE, "ERROR: cannot read name table\n");
        fprintf(stderr, "ERROR: cannot read name table\n");

        return NULL;
    }

    // reading tree
    node_t * root = readTreeFromIRrecursive(fe, &cur_pos);

    free(buffer);

    return root;
}

static int readSignature(fe_context_t * fe, const char ** cur_pos)
{
    assert(fe);
    assert(cur_pos);
    assert(*cur_pos);

    char sign_str[SIGN_MAX_LEN] = "";

    int shift = 0;
    sscanf(*cur_pos, "%[^\n]%n", sign_str, &shift);
    *cur_pos += shift;

    logPrint(LOG_DEBUG, "read signature: %s\n", sign_str);

    if (strcmp(sign_str, SIGN_STRING) == 0)
        return 1;

    return 0;
}

static int readNameTable(fe_context_t * fe, const char ** cur_pos)
{
    assert(fe);
    assert(cur_pos);
    assert(*cur_pos);

    int shift = 0;
    char buffer[BUFFER_LEN] = "";

    sscanf(*cur_pos, " %[^{ ] {%n", buffer, &shift);
    *cur_pos += shift;

    logPrint(LOG_DEBUG, "read name table name: '%s'\n", buffer);

    if (strcmp(buffer, "nametable") != 0)
        return 0;

    shift = 0;

    while (shift == 0){
        char name_buf[BUFFER_LEN] = "";
        size_t index = 0;

        sscanf(*cur_pos, " %zu : \"%[^\"]\" ;%n", &index, name_buf, &shift);
        *cur_pos += shift;

        logPrint(LOG_DEBUG, "scanned name: %04zu, \"%s\";\n", index, name_buf);

        strcpy(fe->ids[index].name, name_buf);
        fe->id_size++;

        shift = 0;
        sscanf(*cur_pos, " }%n", &shift);
    }
    *cur_pos += shift;

    logPrint(LOG_DEBUG, "successfully scanned nametable\n");

    return 1;
}

static node_t * readTreeFromIRrecursive(fe_context_t * fe, const char ** cur_pos)
{
    assert(fe);
    assert(cur_pos);
    assert(*cur_pos);

    int shift = 0;

    sscanf(*cur_pos, " { }%n", &shift);
    if (shift != 0){
        *cur_pos += shift;

        return NULL;
    }

    char type_str[MAX_ELEM_TYPE_NAME_LEN] = "";
    shift = 0;

    sscanf(*cur_pos, " { %[^ :] :%n", type_str, &shift);
    *cur_pos += shift;

    union value val = {};

    if (strcmp(type_str, "NUM") == 0){
        double number = 0.;

        sscanf(*cur_pos, " %lg }%n", &number, &shift);
        *cur_pos += shift;

        node_t * num_node = fe->cur_node;
        fe->cur_node++;

        num_node->type = NUM;
        num_node->val.number = number;

        num_node->left  = NULL;
        num_node->right = NULL;

        return num_node;
    }

    if (strcmp(type_str, "IDR") == 0){
        unsigned int id_index = 0;

        sscanf(*cur_pos, " %u }%n", &id_index, &shift);
        *cur_pos += shift;

        node_t * idr_node = fe->cur_node;
        fe->cur_node++;

        idr_node->type = IDR;
        idr_node->val.id = id_index;

        idr_node->left  = NULL;
        idr_node->right = NULL;

        return idr_node;
    }

    // if (strmcp(type_str, "OPR") == 0)
    enum oper op_num = NO_OP;

    sscanf(*cur_pos, " %d %n", &op_num, &shift);
    *cur_pos += shift;

    node_t * opr_node = fe->cur_node;
    fe->cur_node++;

    opr_node->type = OPR;
    opr_node->val.op = op_num;

    opr_node->left  = readTreeFromIRrecursive(fe, cur_pos);
    opr_node->right = readTreeFromIRrecursive(fe, cur_pos);

    sscanf(*cur_pos, " }%n", &shift);
    *cur_pos += shift;

    return opr_node;
}

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

static size_t getFileSize(const char * file_name)
{
    assert(file_name);

    struct stat st = {};
    stat(file_name, &st);

    size_t file_len = st.st_size;

    return file_len;
}
