#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "backend.h"
#include "logger.h"

const size_t MAX_NODES_NUM = 1024;

int main()
{
    mkdir("backend_logs", 0777);

    logStart("backend_logs/log.html", LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    be_context_t backend = backendInit(MAX_NODES_NUM, "compiled.asm", "out.txt");

    backendDtor(&backend);

    logExit();

    return 0;
}
