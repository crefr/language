#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
#include "frontend.h"
#include "tree.h"

const size_t MAX_CODE_LEN   = 1024;
const size_t MAX_TOKEN_NUM  = 1024;

int main()
{
    mkdir("logs", 0777);

    logStart("logs/log.html", LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    fe_context_t fe = frontendInit(MAX_TOKEN_NUM);

    char * str = readProgramText("program.txt");

    lexicalAnalysis(&fe, str);
    frontendDump(&fe);

    node_t * tree = parseCode(&fe);
    assert(tree);

    treeDumpGraph(&fe, tree);

    printTreePrefix(&fe, tree);
    printf("\n");

    frontendDtor(&fe);

    logExit();
    free(str);

    return 0;
}
