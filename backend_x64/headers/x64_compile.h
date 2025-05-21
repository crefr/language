#ifndef X64_COMPILE_INCLUDED
#define X64_COMPILE_INCLUDED

#include "backend_x64.h"

void compile(backend_ctx_t * ctx, const char * asm_file_name, const char * elf_file_name, const char * std_lib_file_name);

#endif
