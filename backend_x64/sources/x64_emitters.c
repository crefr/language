#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include "x64_emitters.h"

const uint8_t REX_CODE = 0x40;

const uint8_t REX_W = REX_CODE | (1 << 3);  // 1 = 64 Bit Operand Size
const uint8_t REX_R = REX_CODE | (1 << 2);  // Extension of the ModR/M reg field
const uint8_t REX_X = REX_CODE | (1 << 1);  // Extension of the SIB index field
const uint8_t REX_B = REX_CODE | (1 << 0);  // Extension of the ModR/M r/m field, SIB base field, or Opcode reg field


static size_t emit_bytes_func(emit_ctx_t * ctx, size_t num_of_bytes, ...)
{
    va_list args;
    va_start(args, num_of_bytes);

    printf("emitting\n");

    if (ctx->emitting){
        printf("emitting, BITCH!\n");
        for (size_t byte_index = 0; byte_index < num_of_bytes; byte_index++){
            uint8_t byte = (uint8_t)va_arg(args, int);
            fputc(byte, ctx->bin_file);
        }
    }

    va_end(args);

    return num_of_bytes;
}


static size_t emit_imm32_func(emit_ctx_t * ctx, int32_t imm)
{
    if (ctx->emitting)
        fwrite(&imm, sizeof(imm), 1, ctx->bin_file);

    return sizeof(imm);
}


static size_t emit_imm64_func(emit_ctx_t * ctx, int64_t imm)
{
    if (ctx->emitting)
        fwrite(&imm, sizeof(imm), 1, ctx->bin_file);

    return sizeof(imm);
}


static uint8_t modRM(uint8_t mod, uint8_t reg, uint8_t rm)
{
    return (mod << 6) | (reg << 3) | (rm);
}

// magic for counting arguments (up to 5)
#define COUNT_ARGS(...) COUNT_ARGS_IMPL(__VA_ARGS__, 5,4,3,2,1,0)
#define COUNT_ARGS_IMPL(_1,_2,_3,_4,_5,N,...) N

#define emit_bytes(...) emit_bytes_func(ctx, COUNT_ARGS(__VA_ARGS__), __VA_ARGS__  )
#define emit_imm32(imm) emit_imm32_func(ctx, imm)
#define emit_imm64(imm) emit_imm64_func(ctx, imm)

#define check_src_reg(reg, rex) do{if (reg >= R_R8) {reg-=8; rex|=REX_R;}} while(0)
#define check_dst_reg(reg, rex) do{if (reg >= R_R8) {reg-=8; rex|=REX_B;}} while(0)

#define asm_emit(...) fprintf(ctx->asm_file, "\t\t" __VA_ARGS__)


/******************** PUSH ********************/
// push reg64
size_t emit_push_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("push %s\n", reg_names[reg]);

    // r8+ registers
    if (reg >= 8)
        return emit_bytes(REX_B, (uint8_t)(0x50 + (reg - 8)));

    return emit_bytes((uint8_t)(0x50 + reg));
}

// push imm32
size_t emit_push_imm32(emit_ctx_t * ctx, int32_t imm32)
{
    asm_emit("push %d\n", imm32);

    size_t bytes_emitted = 0;

    bytes_emitted += emit_bytes((uint8_t)0x68);
    bytes_emitted += emit_imm32(imm32);

    return bytes_emitted;
}

// push QWORD[reg + imm32]
size_t emit_push_mem(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("push QWORD[%s+(%d)]\n", reg_names[reg], imm32);

    size_t bytes_emitted = 0;

    if (reg < 8)
        bytes_emitted += emit_bytes((uint8_t)0xFF, modRM(0b10, 6, reg));
    else
        bytes_emitted += emit_bytes(REX_B, (uint8_t)0xFF, modRM(0b10, 6, reg - 8));

    bytes_emitted += emit_imm32(imm32);

    return bytes_emitted;
}
/**********************************************/


/******************** POP ********************/
// pop reg64
size_t emit_pop_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("pop %s\n", reg_names[reg]);

    // r8+ registers
    if (reg >= 8)
        return emit_bytes(REX_B, (uint8_t)(0x58 + (reg - 8)));

    return emit_bytes((uint8_t)(0x58 + reg));
}


// pop QWORD[reg + imm32]
size_t emit_pop_mem(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("push QWORD[%s+(%d)]\n", reg_names[reg], imm32);

    size_t bytes_emitted = 0;

    if (reg < 8)
        bytes_emitted += emit_bytes((uint8_t)0x8F, modRM(0b10, 0, reg));
    else
        bytes_emitted += emit_bytes(REX_B, (uint8_t)0x8F, modRM(0b10, 0, reg - 8));

    bytes_emitted += emit_imm32(imm32);

    return bytes_emitted;
}
/*********************************************/



/******************** SUB, ADD ********************/
// sub reg64, imm32
size_t emit_sub_reg_imm32(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("sub %s, %d\n", reg_names[reg], imm32);

    uint8_t rex = REX_W;

    check_dst_reg(reg, rex);

    size_t bytes_emitted = 0;

    bytes_emitted += emit_bytes(rex, (uint8_t)0x81, modRM(0b11, 5, reg));
    bytes_emitted += emit_imm32(imm32);

    return bytes_emitted;
}


// sub reg64, reg64
size_t emit_sub_reg_reg(emit_ctx_t * ctx, int dest, int src)
{
    asm_emit("sub %s, %s\n", reg_names[dest], reg_names[src]);

    uint32_t rex = REX_W;
    check_dst_reg(dest, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, (uint8_t)0x29, modRM(0b11, src, dest));
}



// add reg64, imm32
size_t emit_add_reg_imm32(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("add %s, %d\n", reg_names[reg], imm32);

    uint8_t rex = REX_W;

    check_dst_reg(reg, rex);

    size_t bytes_emitted = 0;

    bytes_emitted += emit_bytes(rex, (uint8_t)0x81, modRM(0b11, 0, reg));
    bytes_emitted += emit_imm32(imm32);

    return bytes_emitted;
}


// sub reg64, reg64
size_t emit_add_reg_reg(emit_ctx_t * ctx, int dest, int src)
{
    asm_emit("add %s, %s\n", reg_names[dest], reg_names[src]);

    uint32_t rex = REX_W;
    check_dst_reg(dest, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, (uint8_t)0x01, modRM(0b11, src, dest));
}
/**************************************************/


/******************** IDIV, IMUL ********************/
// idiv reg64
size_t emit_idiv_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("idiv %s\n", reg_names[reg]);

    uint8_t rex = REX_W;
    check_dst_reg(reg, rex);

    return emit_bytes(rex, (uint8_t)0xF7, modRM(0b11, 7, reg));
}


// imul reg64
size_t emit_imul_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("imul %s\n", reg_names[reg]);

    uint8_t rex = REX_W;
    check_dst_reg(reg, rex);

    return emit_bytes(rex, (uint8_t)0xF7, modRM(0b11, 5, reg));
}
/****************************************************/


/******************** MOV ********************/
// mov reg64, reg64
size_t emit_mov_reg_reg(emit_ctx_t * ctx, int dst, int src)
{
    asm_emit("mov %s, %s\n", reg_names[dst], reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, (uint8_t)0x89, modRM(0b11, src, dst));
}


// mov QWORD[reg64 + imm32], reg64
size_t emit_mov_mem_reg(emit_ctx_t * ctx, int dst, int32_t imm32, int src)
{
    asm_emit("mov QWORD[%s+(%d)], %s\n", reg_names[dst], imm32, reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    size_t emitted_bytes = 0;

    emitted_bytes += emit_bytes(rex, (uint8_t)0x89, modRM(0b10, src, dst));
    emitted_bytes += emit_imm32(imm32);

    return emitted_bytes;
}


// mov reg64, imm64
size_t emit_mov_reg_imm(emit_ctx_t * ctx, int reg, int64_t imm64)
{
    asm_emit("mov %s, %ld\n", reg_names[reg], imm64);

    uint8_t rex = REX_W;
    check_dst_reg(reg, rex);

    size_t emitted_bytes = 0;

    emitted_bytes += emit_bytes(rex, (uint8_t)(0xB8 + reg));
    emitted_bytes += emit_imm64(imm64);

    return emitted_bytes;
}
/*********************************************/


/****************** JMP, JZ, CALL ******************/
// jmp imm32 (near)
size_t emit_jmp_rel32(emit_ctx_t * ctx, int32_t rel32)
{
    asm_emit("jmp $ + 5 + (%d)\n", rel32);

    size_t emitted_bytes = 0;
    emitted_bytes += emit_bytes((uint8_t)0xE9);
    emitted_bytes += emit_imm32(rel32);

    return emitted_bytes;
}

// jz imm32 (near)
size_t emit_jz_rel32(emit_ctx_t * ctx, int32_t rel32)
{
    asm_emit("jz $ + 6 + (%d)\n", rel32);

    size_t emitted_bytes = 0;
    emitted_bytes += emit_bytes((uint8_t)0x0F, (uint8_t)0x84);
    emitted_bytes += emit_imm32(rel32);

    return emitted_bytes;
}


// call imm32 (near)
size_t emit_call_rel32(emit_ctx_t * ctx, int32_t rel32)
{
    asm_emit("call $ + 5 + (%d)\n", rel32);

    size_t emitted_bytes = 0;
    emitted_bytes += emit_bytes((uint8_t)0xE8);
    emitted_bytes += emit_imm32(rel32);

    return emitted_bytes;
}
/***************************************************/


// test reg64, reg64
size_t emit_test_reg_reg(emit_ctx_t * ctx, int dst, int src)
{
    asm_emit("test %s, %s\n", reg_names[dst], reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, (uint8_t)0x85, modRM(0b11, src, dst));
}


// ret
size_t emit_ret(emit_ctx_t * ctx)
{
    asm_emit("ret\n");
    return emit_bytes((uint8_t)0xC3);
}

// cqo
size_t emit_cqo(emit_ctx_t * ctx)
{
    asm_emit("cqo\n");

    return emit_bytes((uint8_t)0x48, (uint8_t)0x99);
}

// syscall
size_t emit_syscall(emit_ctx_t * ctx)
{
    asm_emit("syscall\n");

    return emit_bytes((uint8_t)0x0F, (uint8_t)0x05);
}


