#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "backend_x64.h"

#define asm_emit(...)         fprintf(ctx->asm_file, "\t\t" __VA_ARGS__)
#define asm_emit_label(...)   fprintf(ctx->asm_file, __VA_ARGS__)
#define asm_emit_comment(...) fprintf(ctx->asm_file, "; " __VA_ARGS__)
#define asm_end_of_block()    fprintf(ctx->asm_file, "\n")


static void emitStdFuncs(backend_ctx_t * ctx, const char * std_lib_file_name);

static void emitStart(backend_ctx_t * ctx);

static void emitExit(backend_ctx_t * ctx);


static void compileSetFrPtr(backend_ctx_t * ctx, IR_block_t * block);

static void compileJmp(backend_ctx_t * ctx, IR_block_t * block);

static void compileAddSubMulDiv(backend_ctx_t * ctx, IR_block_t * block);

static void compileVarDecl(backend_ctx_t * ctx, IR_block_t * block);

static void compileAssign(backend_ctx_t * ctx, IR_block_t * block);

static void compileCall(backend_ctx_t * ctx, IR_block_t * block);

static void compileReturn(backend_ctx_t * ctx, IR_block_t * block);

static void compileCondJmp(backend_ctx_t * ctx, IR_block_t * block);

static void compileLabel(backend_ctx_t * ctx, IR_block_t * block);

static void compilePushImm(backend_ctx_t * ctx, IR_block_t * block);

static void compilePushMem(backend_ctx_t * ctx, IR_block_t * block);

static void compilePopVar(backend_ctx_t * ctx, IR_block_t * block);


void compileFromIR(backend_ctx_t * ctx, const char * asm_file_name, const char * std_lib_file_name)
{
    assert(ctx);
    assert(asm_file_name);
    assert(std_lib_file_name);

    ctx->asm_file = fopen(asm_file_name, "w");
    logPrint(LOG_DEBUG, "\nstarted translating to asm...\n");

    fprintf(ctx->asm_file, "global _start\n");

    emitStdFuncs(ctx, std_lib_file_name);

    emitStart(ctx);

    for (size_t block_index = 0; block_index < ctx->IR.size; block_index++){
        IR_block_t * block = ctx->IR.blocks + block_index;

        switch(block->type){
            case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
                compileAddSubMulDiv(ctx, block); break;

            case IR_CALL: compileCall(ctx, block); break;

            case IR_RET: compileReturn(ctx, block); break;

            case IR_COND_JMP: compileCondJmp(ctx, block); break;

            case IR_JMP: compileJmp(ctx, block); break;

            case IR_LABEL: compileLabel(ctx, block); break;

            case IR_PUSH_IMM: compilePushImm(ctx, block); break;

            case IR_PUSH_MEM: compilePushMem(ctx, block); break;

            case IR_SET_FR_PTR: compileSetFrPtr(ctx, block); break;
        }
    }

    emitExit(ctx);

    logPrint(LOG_DEBUG, "\nsuccessfully translated to asm!\n");
    fclose(ctx->asm_file);
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


static void emitStart(backend_ctx_t * ctx)
{
    asm_emit_comment("===================== STARTING TRANSLATION =====================\n");


    asm_emit_label("_start:\n");
    asm_emit("mov rbx, rsp\n");

    asm_end_of_block();
}


static void emitExit(backend_ctx_t * ctx)
{
    asm_emit_comment("\t--- EXITING ---\n");

    asm_emit("mov rsp, rbx\n");

    asm_emit("mov rax, 0x3c\n");
    asm_emit("mov rdi, 0x00\n");

    asm_emit("syscall\n");
}


static void compileSetFrPtr(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit("push rbp");
    asm_emit("mov rbp, rsp");

    asm_end_of_block();
}


static void compileJmp(backend_ctx_t * ctx, IR_block_t * block)
{
    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit("jmp %s\n", label_block->label_name);
    asm_end_of_block();
}


static void compileCondJmp(backend_ctx_t * ctx, IR_block_t * block)
{
    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit_comment("--- COND CHECK ---\n");

    asm_emit("pop rsi\n");
    asm_emit("test rsi, rsi");
    asm_emit("jz %s\n", label_block->label_name);

    asm_end_of_block();
}


static void compileLabel(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_label("%s:\n", block->lable->name);
}


static void compilePushImm(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit("push %ld\n", block->imm_val);
}


static void compilePushMem(backend_ctx_t * ctx, IR_block_t * block)
{
    if (block->addr->is_global)
        asm_emit("push QWORD [rbx+(%ld)]\t", block->addr->rel_addr);
    else
        asm_emit("push QWORD [rbp+(%ld)]\t", block->addr->rel_addr);

    asm_emit_comment("%s\n", ctx->id_table[block->addr->name_index].name);
}


static void compilePopVar(backend_ctx_t * ctx, IR_block_t * block)
{
    if (block->addr->is_global)
        asm_emit("pop QWORD [rbx+(%ld)] \t", block->addr->rel_addr);
    else
        asm_emit("pop QWORD [rbp+(%ld)] \t", block->addr->rel_addr);

    asm_emit_comment("%s\n", ctx->id_table[block->addr->name_index].name);
}


static void compileVarDecl(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit("sub rsp, 8\t\t\t\t");
    asm_emit_comment("%s\n", ctx->id_table[block->addr->name_index].name);
}


static void compileAssign(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_comment("\t--- ASSIGN ---\n");
    compilePopVar(ctx, block);

    asm_end_of_block();
}


static void compileAddSubMulDiv(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_comment("\t--- ADD, SUB, MUL or DIV ---\n");

    if (block->type == IR_DIV)
        asm_emit("xor rdx, rdx\n");

    asm_emit("pop rcx\n");
    asm_emit("pop rax\n");

    switch(block->type){
        case IR_ADD: asm_emit("add rax, rcx\n"); break;
        case IR_SUB: asm_emit("sub rax, rcx\n"); break;
        case IR_MUL: asm_emit("imul rcx\n"); break;
        case IR_DIV: asm_emit("idiv rcx\n"); break;
    }

    asm_emit("push rax\n");

    asm_emit_comment("\t----------------------------\n");
    asm_end_of_block();
}


static void compileCall(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_comment("\t--- CALLING %s ---\n", func_name);

    IR_block_t * label_block = ctx->IR.blocks + block->label_block_idx;

    asm_emit("call %s\n", label_block->label_name);
    asm_emit("add %zu\n", label_block->arg_num * 8);
    asm_emit("push rax\n");

    asm_emit_comment("\t--- END OF CALLING %s ---\n", func_name);
    asm_end_of_block();
}


static void compileReturn(backend_ctx_t * ctx, IR_block_t * block)
{
    asm_emit_comment("\t --- RETURN ---\n");

    asm_emit("pop rax\n");
    asm_emit("mov rsp, rbp\n");
    asm_emit("pop rbp\n");
    asm_emit("ret\n");

    asm_end_of_block();
}

