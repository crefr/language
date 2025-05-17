#include <stdio.h>
#include <stdlib.h>

#include "backend_x64.h"

int main(int argc, char ** argv)
{
    if (argc != 3){
        fprintf(stderr, "X64 BACKEND: incorrect number of args given!\n");
        return 0;
    }

    backend_ctx_t backend = backendInit(argv[1]);

    backendDestroy(&backend);

    return 0;
}
