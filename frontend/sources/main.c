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
        const char * ir_file_name   = "out.ast";
        const char * code_file_name = "generated_code.txt";

        if (argc > 2){
            ir_file_name   = argv[2];
            code_file_name = argv[3];
        }

        reverseFrontendRun(ir_file_name, code_file_name);

        return 0;
    }

    const char * code_file_name = "program.txt";
    const char * ir_file_name   = "out.ast";

    if (argc > 1){
        code_file_name = argv[1];
        ir_file_name   = argv[2];
    }

    frontendRun(code_file_name, ir_file_name);

    logExit();

    return 0;
}
