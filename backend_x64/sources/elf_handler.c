#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <elf.h>

size_t writeSimpleElfHeader(FILE * elf_file, size_t code_size)
{
    assert(elf_file);

    /*
    structure will be like this:

    Elf64_Ehdr
    Elf64_Phdr
    <code>
    */

    const size_t phdr_offset = sizeof(Elf64_Ehdr);
    const size_t code_offset = phdr_offset + sizeof(Elf64_Phdr);

    const size_t code_segment_start = 0x401000;
    const size_t entry_point = code_segment_start + code_offset;

    // ELF main header
    Elf64_Ehdr elf_hdr = {
        .e_ident = {
            [EI_MAG0] = ELFMAG0,
            [EI_MAG1] = ELFMAG1,
            [EI_MAG2] = ELFMAG2,
            [EI_MAG3] = ELFMAG3,

            [EI_CLASS]   = ELFCLASS64,
            [EI_DATA]    = ELFDATA2LSB,
            [EI_VERSION] = EV_CURRENT,
            // [EI_OSABI]   = ELFOSABI_LINUX
        },
        .e_type    = ET_EXEC,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,

        .e_entry = entry_point,             // standard entry address
        .e_phoff = phdr_offset,
        .e_ehsize = sizeof(Elf64_Ehdr),
        .e_phentsize = sizeof(Elf64_Phdr),
        .e_phnum = 1                        // only one program header
    };

    // ELF program segment header
    Elf64_Phdr seg_hdr = {
        .p_type  = PT_LOAD,             // loadable
        .p_flags = PF_X | PF_R,         // readable and executable
        .p_offset = 0,
        .p_vaddr  = code_segment_start,
        .p_paddr  = code_segment_start,
        .p_filesz = code_size + code_offset,          // size of code
        .p_memsz  = code_size + code_offset,          // size of code too?
        .p_align  = 0x1000
    };

    // writing to file
    fwrite(&elf_hdr, sizeof(elf_hdr), 1, elf_file);
    fwrite(&seg_hdr, sizeof(seg_hdr), 1, elf_file);

    return code_offset;
}
