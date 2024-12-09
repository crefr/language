#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
#include "frontend.h"

const size_t MAX_TOKEN_NUM = 1024;

int main()
{
    mkdir("logs", 0777);

    logStart("logs/log.html", LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    fe_context_t fe = frontendInit(MAX_TOKEN_NUM);

    lexicalAnalysis(&fe, "2");

    node_t * tree = parseCode(&fe);

    frontendDtor(&fe);

    logExit();

    return 0;
}
