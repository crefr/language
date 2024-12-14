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

        node_t * tree = readTreeFromIR(&fe, "out.txt");

        treeDumpGraph(&fe, tree);

        frontendDtor(&fe);

        logExit();

        return 0;
    }

    fe_context_t fe = frontendInit(MAX_TOKEN_NUM);

    char * str = readProgramText("program.txt");

    lexicalAnalysis(&fe, str);
    frontendDump(&fe);

    node_t * tree = parseCode(&fe);
    assert(tree);

    treeDumpGraph(&fe, tree);

    printTreePrefix(&fe, tree);
    printf("\n");

    FILE * out = fopen("out.txt", "w");
    writeTreeToFile(&fe, tree, out);
    fclose(out);

    frontendDtor(&fe);

    logExit();
    free(str);

    return 0;
}
