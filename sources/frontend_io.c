#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "frontend.h"
#include "logger.h"

char * readProgramText(const char * file_name)
{
    logPrint(LOG_DEBUG, "reading program text\n");

    assert(file_name);

    struct stat st = {};
    stat(file_name, &st);

    size_t file_len = st.st_size;
    logPrint(LOG_DEBUG, "file size: %zu\n", file_len);

    FILE * file = fopen(file_name, "r");
    assert(file);

    char * code_str = (char *)calloc(file_len, *code_str);
    fread(code_str, sizeof(*code_str), file_len, file);

    fclose(file);

    return code_str;
}
