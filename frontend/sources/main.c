#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
#include "reverse_frontend.h"
#include "frontend.h"
#include "tree.h"

int main(int argc, char ** argv)
{
    mkdir(LOG_FOLDER_NAME, 0777);

    logStart(LOG_FILE_NAME, LOG_DEBUG_PLUS, LOG_HTML);
    logCancelBuffer();

    if (argc > 1 && (strcmp(argv[1], "-1") == 0)){
        reverseFrontendRun("out.txt", "generated_code.txt");

        return 0;
    }

    frontendRun("program.txt", "out.txt");

    logExit();

    return 0;
}
