#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "IR_handler.h"
#include "logger.h"

const char * const SIGN_STRING  = "IR312:1";
const size_t SIGN_MAX_LEN = 32;
const size_t BUFFER_LEN   = 32;

static void writeNameTable(tree_context_t * tree, FILE * out_file);

static void writeTreeToFileRecursive(tree_context_t * tree, node_t * node, FILE * out_file, const size_t tab_num);

static size_t getFileSize(const char * file_name);

static int readSignature(tree_context_t * tree, const char ** cur_pos);

static node_t * readTreeFromIRrecursive(tree_context_t * tree, const char ** cur_pos);

static int readNameTable(tree_context_t * tree, const char ** cur_pos);

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

node_t * readTreeFromIR(tree_context_t * tree, const char * file_name)
{
    assert(file_name);
    assert(tree);

    logPrint(LOG_DEBUG, "reading tree from IR\n");

    size_t file_len = getFileSize(file_name) + 1;

    FILE * file = fopen(file_name, "r");
    assert(file);

    char * buffer = (char *)calloc(file_len, sizeof(*buffer));
    fread(buffer, sizeof(*buffer), file_len, file);

    fclose(file);

    const char * cur_pos = buffer;

    // checking signature
    if (! readSignature(tree, &cur_pos)){
        logPrint(LOG_RELEASE, "ERROR: invalid signature\n");
        fprintf(stderr, "ERROR: invalid signature\n");

        return NULL;
    }

    // reading name table
    if (! readNameTable(tree, &cur_pos)){
        logPrint(LOG_RELEASE, "ERROR: cannot read name table\n");
        fprintf(stderr, "ERROR: cannot read name table\n");

        return NULL;
    }

    // reading tree
    node_t * root = readTreeFromIRrecursive(tree, &cur_pos);

    free(buffer);

    return root;
}

static int readSignature(tree_context_t * tree, const char ** cur_pos)
{
    assert(tree);
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

static int readNameTable(tree_context_t * tree, const char ** cur_pos)
{
    assert(tree);
    assert(cur_pos);
    assert(*cur_pos);

    int shift = 0;

    size_t nametable_size = 0;
    sscanf(*cur_pos, " NAMETABLE size: %zu {%n", &nametable_size, &shift);

    // allocating memory for name table
    tree->ids = (idr_t *)calloc(nametable_size, sizeof(idr_t));
    tree->id_size = nametable_size;

    *cur_pos += shift;

    shift = 0;

    while (shift == 0){
        char name_buf[BUFFER_LEN] = "";
        char type_buf[BUFFER_LEN] = "";
        size_t index = 0;

        size_t num_of_args = 0;

        sscanf(*cur_pos, " %zu : \"%[^\"]\" , %[^ ,;] , %zu ;%n", &index, name_buf, type_buf, &num_of_args, &shift);
        *cur_pos += shift;

        logPrint(LOG_DEBUG, "scanned name: %04zu, \"%s\", %s;\n", index, name_buf, type_buf);

        strcpy(tree->ids[index].name, name_buf);
        tree->ids[index].num_of_args = num_of_args;

        if (strcmp(type_buf, "FUNC") == 0)
            tree->ids[index].type = FUNC;
        else
            tree->ids[index].type = VAR;

        shift = 0;
        sscanf(*cur_pos, " }%n", &shift);
    }
    *cur_pos += shift;

    logPrint(LOG_DEBUG, "successfully scanned nametable\n");

    return 1;
}

static node_t * readTreeFromIRrecursive(tree_context_t * tree, const char ** cur_pos)
{
    assert(tree);
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

        node_t * num_node = tree->cur_node;
        tree->cur_node++;

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

        node_t * idr_node = tree->cur_node;
        tree->cur_node++;

        idr_node->type = IDR;
        idr_node->val.id = id_index;

        idr_node->left  = NULL;
        idr_node->right = NULL;

        return idr_node;
    }

    // if (strmcp(type_str, "OPR") == 0)
    char op_buffer[MAX_IR_OPER_NAME_LEN] = "";

    sscanf(*cur_pos, " %[^\n {] %n", op_buffer, &shift);
    *cur_pos += shift;

    enum oper op_num = NO_OP;
    // searching op_num by the name
    // TODO: may be hashtable?

    for (size_t oper_index = 0; oper_index < oper_names_num; oper_index++){
        if (strcmp(op_buffer, oper_names[oper_index].name) == 0){
            op_num = oper_names[oper_index].op_num;
            break;
        }
    }

    if (op_num == TEXT)
        fprintf(stderr, "WARNING: TEXT operator is not supported\n", op_buffer);

    if (op_num == NO_OP)
        fprintf(stderr, "WARNING: unknown operator '%s' - program is unpredictable (tree writing would be incorrect)\n", op_buffer);

    node_t * opr_node = tree->cur_node;
    tree->cur_node++;

    opr_node->type = OPR;
    opr_node->val.op = op_num;

    opr_node->left  = readTreeFromIRrecursive(tree, cur_pos);
    opr_node->right = readTreeFromIRrecursive(tree, cur_pos);

    sscanf(*cur_pos, " }%n", &shift);
    *cur_pos += shift;

    return opr_node;
}

void writeTreeToFile(tree_context_t * tree, node_t * root, FILE * out_file)
{
    assert(tree);
    assert(root);
    assert(out_file);

    fprintf(out_file, SIGN_STRING);
    fprintf(out_file, "\n");

    writeNameTable(tree, out_file);

    writeTreeToFileRecursive(tree, root, out_file, 0);

    fprintf(out_file, "\n");
}

static void writeNameTable(tree_context_t * tree, FILE * out_file)
{
    assert(tree);
    assert(out_file);

    fprintf(out_file, "NAMETABLE size: %zu {\n", tree->id_size);

    for (size_t id_index = 0; id_index < tree->id_size; id_index++){
        const char * type_str = (tree->ids[id_index].type == VAR) ? "VAR" : "FUNC";
        fprintf(out_file, "\t%04zu: \"%s\", %s, %zu;\n", id_index, tree->ids[id_index].name, type_str, tree->ids[id_index].num_of_args);
    }

    fprintf(out_file, "}\n");
}

static void writeTreeToFileRecursive(tree_context_t * tree, node_t * node, FILE * out_file, const size_t tab_num)
{
    assert(tree);
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

    // searching name for this operator
    const char * op_name = NULL;
    for (size_t oper_index = 0; oper_index < oper_names_num; oper_index++){
        if (oper_names[oper_index].op_num == oper.num){
            op_name = oper_names[oper_index].name;
            break;
        }
    }
    assert(op_name);

    fprintf(out_file, "{OPR:%s\n", op_name);

    writeTreeToFileRecursive(tree, node->left , out_file, tab_num + 1);
    writeTreeToFileRecursive(tree, node->right, out_file, tab_num + 1);

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
