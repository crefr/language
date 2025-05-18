#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "backend_x64.h"
#include "logger.h"

const char * const LOG_FOLDER_NAME = "logs";
const char * const LOG_FILE_NAME   = "logs/log.html";

int main(int argc, char ** argv)
{
    if (argc != 3){
        fprintf(stderr, "X64 BACKEND: incorrect number of args given!\n");
        return 0;
    }

    mkdir(LOG_FOLDER_NAME, 0777);
    logStart(LOG_FILE_NAME, LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    backend_ctx_t backend = backendInit(argv[1]);

    makeAssemblyCode(&backend, argv[2], "std_funcs.asm");

    backendDestroy(&backend);

    return 0;
}
