#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include <sys/stat.h>

#include "backend_x64.h"
#include "x64_compile.h"
#include "x64_emitters.h"
#include "elf_handler.h"
#include "logger.h"

#define asm_emit_label(...)   if (ctx->emit->emitting) fprintf(ctx->emit->asm_file, __VA_ARGS__)
#define asm_emit_comment(...) if (ctx->emit->emitting) fprintf(ctx->emit->asm_file, "; " __VA_ARGS__)
#define asm_end_of_block()    if (ctx->emit->emitting) fprintf(ctx->emit->asm_file, "\n")

#define BLOCK_START     size_t block_size = 0
#define EMIT(emit_func, ...) block_size += emit_func (ctx->emit ,##__VA_ARGS__)
#define BLOCK_RET       return block_size

static size_t calculateAddresses(backend_ctx_t * ctx, size_t start_addr);

static size_t readStdFuncsAddresses(backend_ctx_t * ctx, FILE * std_lib_file, size_t std_lib_code_size);

static size_t compileFromIR(backend_ctx_t * ctx, size_t start_addr);


static void emitBinStdFuncs(FILE * std_funcs_bin_file, FILE * bin_file, size_t std_funcs_code_size);

static size_t emitStart(backend_ctx_t * ctx, IR_block_t * block);

static size_t emitExit(backend_ctx_t * ctx, IR_block_t * block);


static size_t compileSetFrPtr(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileJmp(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileAddSubMulDiv(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileVarDecl(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileAssign(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileCall(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileReturn(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileCondJmp(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileLabel(backend_ctx_t * ctx, IR_block_t * block);

static size_t compilePushImm(backend_ctx_t * ctx, IR_block_t * block);

static size_t compilePushMem(backend_ctx_t * ctx, IR_block_t * block);

static size_t compilePopVar(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileIn(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileOut(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileSqrt(backend_ctx_t * ctx, IR_block_t * block);

static size_t compileCmp(backend_ctx_t * ctx, IR_block_t * block);


void compile(backend_ctx_t * ctx, const char * asm_file_name, const char * elf_file_name, const char * std_lib_file_name)
{
    assert(ctx);
    assert(asm_file_name);
    assert(std_lib_file_name);

    FILE * emit_asm_file = fopen(asm_file_name, "w");
    FILE * emit_bin_file = fopen(elf_file_name, "wb");

    FILE * std_lib_bin_file = fopen(std_lib_file_name, "rb");

    assert(emit_bin_file);
    setbuf(emit_bin_file, NULL);

    emit_ctx_t emit_ctx = {
        .bin_file = emit_bin_file,
        .asm_file = emit_asm_file,
        .emitting = true
    };

    ctx->emit = &emit_ctx;

    /**** compiling here ****/
    size_t std_lib_code_size = moveToCodeStart(std_lib_bin_file);

    std_lib_code_size = readStdFuncsAddresses(ctx, std_lib_bin_file, std_lib_code_size);

    size_t code_size = calculateAddresses(ctx, std_lib_code_size);

    writeSimpleElfHeader(emit_bin_file, std_lib_code_size, code_size);

    emitBinStdFuncs(std_lib_bin_file, emit_bin_file, std_lib_code_size);
    compileFromIR(ctx, 0);
    /************************/

    fclose(std_lib_bin_file);
    fclose(emit_asm_file);
    fclose(emit_bin_file);

    chmod(elf_file_name, 0755);
}


static size_t readStdFuncsAddresses(backend_ctx_t * ctx, FILE * std_lib_file, size_t std_lib_code_size)
{
    assert(ctx);
    assert(std_lib_file);

    size_t std_in_addr = 0;
    size_t std_out_addr = 0;

    fread(&std_in_addr , sizeof(std_in_addr) , 1, std_lib_file);
    fread(&std_out_addr, sizeof(std_out_addr), 1, std_lib_file);

    std_lib_code_size -= sizeof(std_in_addr) + sizeof(std_out_addr);

    std_in_addr  -= 16;
    std_out_addr -= 16;

    ctx->IR.std_in_addr  = std_in_addr;
    ctx->IR.std_out_addr = std_out_addr;

    logPrint(LOG_DEBUG, "std_in  addr = %zu\n", std_in_addr);
    logPrint(LOG_DEBUG, "std_out addr = %zu\n", std_out_addr);

    return std_lib_code_size - 16;
}


static size_t calculateAddresses(backend_ctx_t * ctx, size_t start_addr)
{
    assert(ctx);

    ctx->emit->emitting = false;

    size_t code_size = compileFromIR(ctx, start_addr);

    ctx->emit->emitting = true;

    return code_size;
}


static size_t compileFromIR(backend_ctx_t * ctx, size_t start_addr)
{
    assert(ctx);

    logPrint(LOG_DEBUG, "\nstarted translating to asm...\n");

    size_t cur_addr = start_addr;

    for (size_t block_index = 0; block_index < ctx->IR.size; block_index++){
        IR_block_t * block = ctx->IR.blocks + block_index;
        size_t block_size = 0;

        switch(block->type){
            case IR_START: block_size = emitStart(ctx, block); break;

            case IR_EXIT: block_size = emitExit(ctx, block); break;

            case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
                block_size = compileAddSubMulDiv(ctx, block); break;

            case IR_EQUAL: case IR_N_EQUAL: case IR_LESS: case IR_LESS_EQ: case IR_GREATER: case IR_GREATER_EQ:
                block_size = compileCmp(ctx, block); break;

            case IR_CALL: block_size = compileCall(ctx, block); break;

            case IR_RET: block_size = compileReturn(ctx, block); break;

            case IR_COND_JMP: block_size = compileCondJmp(ctx, block); break;

            case IR_JMP: block_size = compileJmp(ctx, block); break;

            case IR_LABEL: block_size = compileLabel(ctx, block); break;

            case IR_PUSH_IMM: block_size = compilePushImm(ctx, block); break;

            case IR_PUSH_MEM: block_size = compilePushMem(ctx, block); break;

            case IR_SET_FR_PTR: block_size = compileSetFrPtr(ctx, block); break;

            case IR_VAR_DECL: block_size = compileVarDecl(ctx, block); break;

            case IR_POP_MEM: block_size = compilePopVar(ctx, block); break;

            case IR_IN: block_size = compileIn(ctx, block); break;

            case IR_OUT: block_size = compileOut(ctx, block); break;

            case IR_SQRT: block_size = compileSqrt(ctx, block); break;
        }

        // if we are calculating addresses
        if (ctx->emit->emitting == false)
            block->addr = cur_addr;

        cur_addr += block_size;
    }


    logPrint(LOG_DEBUG, "\nsuccessfully translated to asm!\n");

    return cur_addr;
}


static void emitStdFuncs(backend_ctx_t * ctx, const char * std_lib_file_name)
{
    FILE * std_lib = fopen(std_lib_file_name, "r");

    fseek(std_lib, 0, SEEK_END);
    size_t file_size = ftell(std_lib);
    fseek(std_lib, 0, SEEK_SET);

    char * buffer = (char *)calloc(file_size, sizeof(char));
    fread(buffer, sizeof(char), file_size, std_lib);
    fwrite(buffer, sizeof(char), file_size, ctx->asm_file);

    free(buffer);
    fclose(std_lib);
}

static void emitBinStdFuncs(FILE * std_funcs_bin_file, FILE * bin_file, size_t std_funcs_code_size)
{
    char * buffer = (char *)calloc(std_funcs_code_size, sizeof(char));
    fread(buffer, sizeof(char), std_funcs_code_size, std_funcs_bin_file);
    fwrite(buffer, sizeof(char), std_funcs_code_size, bin_file);

    free(buffer);
}


static size_t emitStart(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("===================== STARTING TRANSLATION =====================\n");
    asm_emit_label("_start:\n");

    EMIT(emit_mov_reg_reg, R_RBX, R_RSP);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t emitExit(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;
    asm_emit_comment("\t--- EXITING ---\n");

    EMIT(emit_mov_reg_reg, R_RSP, R_RBX);
    EMIT(emit_mov_reg_imm, R_RAX, 0x3c);
    EMIT(emit_mov_reg_imm, R_RDI, 0x00);
    EMIT(emit_syscall);

    BLOCK_RET;
}


static size_t compileSetFrPtr(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    EMIT(emit_push_reg, R_RBP);
    EMIT(emit_mov_reg_reg, R_RBP, R_RSP);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileJmp(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit_comment("\t --- to label %s ---\n", label_block->label_name);

    int32_t rel_addr = (int32_t)label_block->addr - (int32_t)block->addr;
    rel_addr -= 5;
    EMIT(emit_jmp_rel32, rel_addr);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileCondJmp(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit_comment("--- COND CHECK ---\n");

    EMIT(emit_pop_reg, R_RSI);
    EMIT(emit_test_reg_reg, R_RSI, R_RSI);

    int32_t rel_addr = (int32_t)label_block->addr - (int32_t)block->addr;
    rel_addr -= block_size + 6;

    asm_emit_comment("\t --- to label %s ---\n", label_block->label_name);
    EMIT(emit_jz_rel32, rel_addr);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileLabel(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_label("%s:\n", block->label_name);

    return 0;
}


static size_t compilePushImm(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    EMIT(emit_push_imm32, block->imm_val);

    BLOCK_RET;
}


static size_t compilePushMem(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t --- push %s ---\n", ctx->id_table[block->var.name_index].name);

    if (block->var.is_global)
        EMIT(emit_push_mem, R_RBX, block->var.rel_addr);
    else
        EMIT(emit_push_mem, R_RBP, block->var.rel_addr);


    BLOCK_RET;
}


static size_t compilePopVar(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t --- pop %s ---\n", ctx->id_table[block->var.name_index].name);

    if (block->var.is_global)
        EMIT(emit_pop_mem, R_RBX, block->var.rel_addr);
    else
        EMIT(emit_pop_mem, R_RBP, block->var.rel_addr);


    BLOCK_RET;
}


static size_t compileVarDecl(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t --- allocating for %s ---\n", ctx->id_table[block->var.name_index].name);
    EMIT(emit_sub_reg_imm32, R_RSP, 8);

    BLOCK_RET;
}


static size_t compileAssign(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- ASSIGN ---\n");
    block_size += compilePopVar(ctx, block);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileAddSubMulDiv(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- ADD, SUB, MUL or DIV ---\n");

    EMIT(emit_pop_reg, R_RCX);
    EMIT(emit_pop_reg, R_RAX);

    if (block->type == IR_DIV)
        EMIT(emit_cqo);

    switch(block->type){
        case IR_ADD: EMIT(emit_add_reg_reg, R_RAX, R_RCX); break;
        case IR_SUB: EMIT(emit_sub_reg_reg, R_RAX, R_RCX); break;
        case IR_MUL: EMIT(emit_imul_reg, R_RCX); break;
        case IR_DIV: EMIT(emit_idiv_reg, R_RCX); break;
    }

    EMIT(emit_push_reg, R_RAX);

    asm_emit_comment("\t----------------------------\n");
    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileCall(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit_comment("\t--- CALLING %s ---\n", label_block->label_name);

    int32_t rel_addr = label_block->addr - block->addr;
    rel_addr -= 5;
    EMIT(emit_call_rel32, rel_addr);

    EMIT(emit_add_reg_imm32, R_RSP, label_block->arg_num * 8);
    EMIT(emit_push_reg, R_RAX);

    asm_emit_comment("\t--- END OF CALLING %s ---\n", label_block->label_name);
    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileReturn(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- RETURN ---\n");

    EMIT(emit_pop_reg, R_RAX);
    EMIT(emit_mov_reg_reg, R_RSP, R_RBP);
    EMIT(emit_pop_reg, R_RBP);
    EMIT(emit_ret);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileIn(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- STANDARD IN CALLING ---\n");

    size_t std_in_rel_addr = ctx->IR.std_in_addr - block->addr - 5;

    EMIT(emit_call_rel32, std_in_rel_addr);

    if (block->var.is_global)
        EMIT(emit_mov_mem_reg, R_RBX, block->var.rel_addr, R_RAX);
    else
        EMIT(emit_mov_mem_reg, R_RBP, block->var.rel_addr, R_RAX);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileOut(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- STANDARD OUT CALLING ---\n");

    size_t std_out_rel_addr = ctx->IR.std_out_addr - block->addr - 5;

    EMIT(emit_call_rel32, std_out_rel_addr);
    EMIT(emit_add_reg_imm32, R_RSP, 8);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileSqrt(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- SQRT ---\n");

    EMIT(emit_pop_reg, R_RAX);
    EMIT(emit_cvtsi2sd_xmm_reg, XMM0, R_RAX);
    EMIT(emit_sqrtsd_xmm_xmm, XMM0, XMM0);
    EMIT(emit_cvtsd2si_reg_xmm, R_RAX, XMM0);
    EMIT(emit_push_reg, R_RAX);

    asm_end_of_block();

    BLOCK_RET;
}


static size_t compileCmp(backend_ctx_t * ctx, IR_block_t * block)
{
    BLOCK_START;

    asm_emit_comment("\t--- < <= > >= == ---\n");
    EMIT(emit_xor_reg_reg, R_RDX, R_RDX);
    EMIT(emit_pop_reg, R_RCX);
    EMIT(emit_pop_reg, R_RAX);
    EMIT(emit_cmp_reg_reg, R_RAX, R_RCX);

    enum cmp_emit_num cmp_num = (enum cmp_emit_num)(block->type - IR_GREATER + EMIT_GREATER);
    EMIT(emit_setcc_reg8, cmp_num, R_RDX);
    EMIT(emit_push_reg, R_RDX);

    asm_end_of_block();

    BLOCK_RET;
}
