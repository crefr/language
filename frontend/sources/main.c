#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
#include "frontend.h"
#include "tree.h"

const size_t MAX_CODE_LEN   = 1024;
const size_t MAX_TOKEN_NUM  = 1024;

int main(int argc, char ** argv)
{
    mkdir("logs", 0777);

    logStart("logs/log.html", LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    if (argc > 1 && (strcmp(argv[1], "-1") == 0)){
        fe_context_t fe = frontendInit(MAX_TOKEN_NUM);

        tree_context_t tr = {};         //TODO: GET RID OF THAT
        tr.cur_node = fe.cur_node;
        tr.ids = fe.ids;
        tr.id_size = fe.id_size;

        node_t * tree = readTreeFromIR(&tr, "../out.txt");

        treeDumpGraph(&tr, tree);
        frontendDump(&fe);

        frontendDtor(&fe);

        logExit();

        return 0;
    }

    fe_context_t fe = frontendInit(MAX_TOKEN_NUM);

    char * str = readProgramText("../program.txt");

    lexicalAnalysis(&fe, str);
    frontendDump(&fe);

    tree_context_t tr = {};         // TODO: GET RID OF THAT
    tr.cur_node = fe.cur_node;
    tr.ids = fe.ids;
    tr.id_size = fe.id_size;

    node_t * tree = parseCode(&fe);
    assert(tree);

    treeDumpGraph(&tr, tree);

    printTreePrefix(&tr, tree);
    printf("\n");

    FILE * out = fopen("../out.txt", "w");


    writeTreeToFile(&tr, tree, out);
    fclose(out);

    frontendDtor(&fe);

    logExit();
    free(str);

    return 0;
}
