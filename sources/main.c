#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
#include "frontend.h"

const size_t MAX_TOKEN_NUM = 1024;

int main()
{
    mkdir("logs/", S_IFDIR);

    FILE * log2 = fopen("logs/bebra.log", "w");
    fprintf(log2, "preved medved\n");
    fclose(log2);


    logStart("logs/log.html", LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    fe_context_t fe = frontendInit(MAX_TOKEN_NUM);

    lexicalAnalysis(&fe, "a+b*5");

    frontendDtor(&fe);

    logExit();

    return 0;
}
