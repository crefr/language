#include <stdio.h>
#include <stdlib.h>

#include "middleend.h"

int main(int argc, char ** argv)
{
    const char * tree_file_name = "out.ast";

    if (argc > 1)
        tree_file_name = argv[1];

    middleendRun(tree_file_name);

    return 0;
}
