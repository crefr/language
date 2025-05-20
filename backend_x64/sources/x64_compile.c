#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "backend_x64.h"
#include "x64_compile.h"
#include "x64_emitters.h"
#include "logger.h"

#define asm_emit(...)         fprintf(ctx->asm_file, "\t\t" __VA_ARGS__)
#define asm_emit_label(...)   fprintf(ctx->asm_file, __VA_ARGS__)
#define asm_emit_comment(...) fprintf(ctx->asm_file, "; " __VA_ARGS__)
#define asm_end_of_block()    fprintf(ctx->asm_file, "\n")

#define EMIT(emit_func, ...) block_size += emit_func (ctx->emit ,##__VA_ARGS__)


static void emitStdFuncs(backend_ctx_t * ctx, const char * std_lib_file_name);

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


void compile(backend_ctx_t * ctx, const char * asm_file_name, const char * std_lib_file_name)
{
    assert(ctx);
    assert(asm_file_name);
    assert(std_lib_file_name);

    FILE * emit_asm_file = fopen("asm_file.asm", "w");
    FILE * emit_bin_file = fopen("bin_file.bin", "wb");
    setbuf(emit_bin_file, NULL);

    emit_ctx_t emit_ctx = {
        .bin_file = emit_bin_file,
        .asm_file = emit_asm_file,
        .emitting = false
    };
    ctx->emit = &emit_ctx;

    ctx->asm_file = fopen(asm_file_name, "w");

    emitStdFuncs(ctx, std_lib_file_name);

    // first time to calculate indexes
    compileFromIR(ctx);

    ctx->emit->emitting = true;

    // second time for compiling
    compileFromIR(ctx);

    fclose(ctx->asm_file);

    fclose(emit_asm_file);
    fclose(emit_bin_file);
}


void compileFromIR(backend_ctx_t * ctx)
{
    assert(ctx);

    logPrint(LOG_DEBUG, "\nstarted translating to asm...\n");

    fprintf(ctx->asm_file, "global _start\n");

    size_t cur_addr = 0;

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


static size_t emitStart(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;
    asm_emit_comment("===================== STARTING TRANSLATION =====================\n");


    asm_emit_label("_start:\n");

    asm_emit("mov rbx, rsp\n");
    EMIT(emit_mov_reg_reg, R_RBX, R_RSP);

    asm_end_of_block();

    return block_size;
}


static size_t emitExit(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;
    asm_emit_comment("\t--- EXITING ---\n");

    asm_emit("mov rsp, rbx\n");
    EMIT(emit_mov_reg_reg, R_RSP, R_RBX);

    asm_emit("mov rax, 0x3c\n");
    EMIT(emit_mov_reg_imm, R_RAX, 0x3c);

    asm_emit("mov rdi, 0x00\n");
    EMIT(emit_mov_reg_imm, R_RDI, 0x00);

    asm_emit("syscall\n");
    EMIT(emit_syscall);

    return block_size;
}


static size_t compileSetFrPtr(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit("push rbp\n");
    EMIT(emit_push_reg, R_RBP);

    asm_emit("mov rbp, rsp\n");
    EMIT(emit_mov_reg_reg, R_RBP, R_RSP);

    asm_end_of_block();

    return block_size;
}


static size_t compileJmp(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit("jmp %s\n", label_block->label_name);

    int32_t rel_addr = (int32_t)label_block->addr - (int32_t)block->addr;
    EMIT(emit_jmp_rel32, rel_addr);

    asm_end_of_block();

    return block_size;
}


static size_t compileCondJmp(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;
    int32_t rel_addr = (int32_t)label_block->addr - (int32_t)block->addr;

    asm_emit_comment("--- COND CHECK ---\n");

    asm_emit("pop rsi\n");
    EMIT(emit_pop_reg, R_RSI);

    asm_emit("test rsi, rsi\n");
    EMIT(emit_test_reg_reg, R_RSI, R_RSI);

    rel_addr += block_size + 6;

    asm_emit("jz %s\n", label_block->label_name);
    EMIT(emit_jz_rel32, rel_addr);

    asm_end_of_block();

    return block_size;
}


static size_t compileLabel(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_label("%s:\n", block->label_name);

    return 0;
}


static size_t compilePushImm(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;
    asm_emit("push %ld\n", block->imm_val);
    EMIT(emit_push_imm32, block->imm_val);

    return block_size;
}


static size_t compilePushMem(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    if (block->var.is_global){
        asm_emit("push QWORD [rbx+(%ld)]\t", block->var.rel_addr);
        EMIT(emit_push_mem, R_RBX, block->var.rel_addr);
    }
    else{
        asm_emit("push QWORD [rbp+(%ld)]\t", block->var.rel_addr);
        EMIT(emit_push_mem, R_RBP, block->var.rel_addr);
    }

    asm_emit_comment("%s\n", ctx->id_table[block->var.name_index].name);

    return block_size;
}


static size_t compilePopVar(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    if (block->var.is_global){
        asm_emit("pop QWORD [rbx+(%ld)] \t", block->var.rel_addr);
        EMIT(emit_pop_mem, R_RBX, block->var.rel_addr);
    }
    else{
        asm_emit("pop QWORD [rbp+(%ld)] \t", block->var.rel_addr);
        EMIT(emit_pop_mem, R_RBP, block->var.rel_addr);
    }

    asm_emit_comment("%s\n", ctx->id_table[block->var.name_index].name);

    return block_size;
}


static size_t compileVarDecl(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit("sub rsp, 8\t\t\t\t");
    EMIT(emit_sub_reg_imm32, R_RSP, 8);

    asm_emit_comment("%s\n", ctx->id_table[block->var.name_index].name);

    return block_size;
}


static size_t compileAssign(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- ASSIGN ---\n");
    block_size += compilePopVar(ctx, block);

    asm_end_of_block();

    return block_size;
}


static size_t compileAddSubMulDiv(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- ADD, SUB, MUL or DIV ---\n");

    asm_emit("pop rcx\n");
    EMIT(emit_pop_reg, R_RCX);

    asm_emit("pop rax\n");
    EMIT(emit_pop_reg, R_RAX);

    if (block->type == IR_DIV){
        asm_emit("cqo\n");
        EMIT(emit_cqo);
    }

    switch(block->type){
        case IR_ADD:
            asm_emit("add rax, rcx\n");
            EMIT(emit_add_reg_reg, R_RAX, R_RCX);
            break;

        case IR_SUB:
            asm_emit("sub rax, rcx\n");
            EMIT(emit_sub_reg_reg, R_RAX, R_RCX);
            break;

        case IR_MUL:
            asm_emit("imul rcx\n");
            EMIT(emit_imul_reg, R_RCX);
            break;

        case IR_DIV:
            asm_emit("idiv rcx\n");
            EMIT(emit_idiv_reg, R_RCX);
            break;
    }

    asm_emit("push rax\n");
    EMIT(emit_push_reg, R_RAX);

    asm_emit_comment("\t----------------------------\n");
    asm_end_of_block();

    return block_size;
}


static size_t compileCall(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;
    int32_t rel_addr = label_block->addr - block->addr;

    asm_emit_comment("\t--- CALLING %s ---\n", label_block->label_name);

    rel_addr += 5;
    asm_emit("call %s\n", label_block->label_name);
    EMIT(emit_call_rel32, rel_addr);

    asm_emit("add rsp, %zu\n", label_block->arg_num * 8);
    EMIT(emit_add_reg_imm32, R_RSP, label_block->arg_num * 8);

    asm_emit("push rax\n");
    EMIT(emit_push_reg, R_RAX);

    asm_emit_comment("\t--- END OF CALLING %s ---\n", label_block->label_name);
    asm_end_of_block();

    return block_size;
}


static size_t compileReturn(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- RETURN ---\n");

    asm_emit("pop rax\n");
    EMIT(emit_pop_reg, R_RAX);

    asm_emit("mov rsp, rbp\n");
    EMIT(emit_mov_reg_reg, R_RSP, R_RBP);

    asm_emit("pop rbp\n");
    EMIT(emit_pop_reg, R_RBP);

    asm_emit("ret\n");
    EMIT(emit_ret);

    asm_end_of_block();

    return block_size;
}


static size_t compileIn(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- STANDARD IN CALLING ---\n");

    asm_emit("call __in_standard_func_please_do_not_name_your_funcs_this_name__\n");
    EMIT(emit_call_rel32, 52552525);

    if (block->var.is_global){
        asm_emit("mov [rbx + (%ld)], rax\n", block->var.rel_addr);
        EMIT(emit_mov_mem_reg, R_RBX, block->var.rel_addr, R_RAX);
    }
    else{
        asm_emit("mov [rbp + (%ld)], rax\n", block->var.rel_addr);
        EMIT(emit_mov_mem_reg, R_RBP, block->var.rel_addr, R_RAX);
    }

    asm_end_of_block();

    return block_size;
}


static size_t compileOut(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- STANDARD OUT CALLING ---\n");

    asm_emit("call __out_standard_func_please_do_not_name_your_funcs_this_name__\n");
    EMIT(emit_call_rel32, 22869);

    asm_emit("add rsp, 8\n");
    EMIT(emit_add_reg_imm32, R_RSP, 8);

    asm_end_of_block();

    return block_size;
}


static size_t compileSqrt(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- SQRT ---\n");

    asm_emit("pop rax\n");
    EMIT(emit_pop_reg, R_RAX);

    asm_emit("cvtsi2sd xmm0, rax\n");
    //!!!!!!!!!!!!!!!!!!!!!!!!!!

    asm_emit("sqrtsd xmm0, xmm0\n");

    asm_emit("cvtsd2si rax, xmm0\n");
    asm_emit("push rax\n");

    asm_end_of_block();

    return block_size;
}


static size_t compileCmp(backend_ctx_t * ctx, IR_block_t * block)
{
    size_t block_size = 0;

    asm_emit_comment("\t--- < <= > >= == ---\n");
    asm_emit("xor rdx, rdx\n");
    EMIT(emit_xor_reg_reg, R_RDX, R_RDX);

    asm_emit("pop rcx\n");
    EMIT(emit_pop_reg, R_RCX);

    asm_emit("pop rax\n");
    EMIT(emit_pop_reg, R_RAX);

    asm_emit("cmp rax, rcx\n");
    EMIT(emit_cmp_reg_reg, R_RAX, R_RCX);

    switch (block->type){
        case IR_EQUAL:      asm_emit("sete  dl\n"); break;
        case IR_N_EQUAL:    asm_emit("setne dl\n"); break;
        case IR_LESS:       asm_emit("setl  dl\n"); break;
        case IR_LESS_EQ:    asm_emit("setle dl\n"); break;
        case IR_GREATER:    asm_emit("setg  dl\n"); break;
        case IR_GREATER_EQ: asm_emit("setge dl\n"); break;
    }

    enum cmp_emit_num cmp_num = (enum cmp_emit_num)(block->type - IR_GREATER + EMIT_GREATER);
    EMIT(emit_setcc_reg8, cmp_num, R_RDX);

    asm_emit("push rdx\n");
    EMIT(emit_push_reg, R_RDX);

    asm_end_of_block();

    return block_size;
}
