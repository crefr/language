#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "x64_compile.h"
#include "backend_x64.h"
#include "logger.h"

const char * const LOG_FOLDER_NAME = "logs";
const char * const LOG_FILE_NAME   = "logs/log.html";


// ARGS
// 1 - ast file name
// 2 - asm file name (for debug)
// 3 - elf file name
// 4 - std lib file name (binary)
int main(int argc, char ** argv)
{
    if (argc != 5){
        fprintf(stderr, "X64 BACKEND: incorrect number of args given!\n");
        return 0;
    }

    mkdir(LOG_FOLDER_NAME, 0777);
    logStart(LOG_FILE_NAME, LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    backend_ctx_t backend = backendInit(argv[1]);

    makeIR(&backend);
    compile(&backend, argv[2], argv[3], argv[4]);

    backendDestroy(&backend);

    return 0;
}
