#ifndef ELF_HANDLER_INCLUDED
#define ELF_HANDLER_INCLUDED

size_t writeSimpleElfHeader(FILE * elf_file, size_t entry_point_offset, size_t code_size);

size_t moveToCodeStart(FILE * elf_file);

#endif
