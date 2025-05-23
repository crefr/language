#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

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

    if (ctx->emitting){
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

#define asm_emit(...) if (ctx->emitting) fprintf(ctx->asm_file, "\t\t" __VA_ARGS__)


/******************** PUSH ********************/
// push reg64
size_t emit_push_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("push %s\n", reg_names[reg]);

    // r8+ registers
    if (reg >= 8)
        return emit_bytes(REX_B, 0x50 + (reg - 8));

    return emit_bytes(0x50 + reg);
}

// push imm32
size_t emit_push_imm32(emit_ctx_t * ctx, int32_t imm32)
{
    asm_emit("push %d\n", imm32);

    size_t bytes_emitted = 0;

    bytes_emitted += emit_bytes(0x68);
    bytes_emitted += emit_imm32(imm32);

    return bytes_emitted;
}

// push QWORD[reg + imm32]
size_t emit_push_mem(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("push QWORD[%s+(%d)]\n", reg_names[reg], imm32);

    size_t bytes_emitted = 0;

    if (reg < 8)
        bytes_emitted += emit_bytes(0xFF, modRM(0b10, 6, reg));
    else
        bytes_emitted += emit_bytes(REX_B, 0xFF, modRM(0b10, 6, reg - 8));

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
        return emit_bytes(REX_B, 0x58 + (reg - 8));

    return emit_bytes(0x58 + reg);
}


// pop QWORD[reg + imm32]
size_t emit_pop_mem(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("pop QWORD[%s+(%d)]\n", reg_names[reg], imm32);

    size_t bytes_emitted = 0;

    if (reg < 8)
        bytes_emitted += emit_bytes(0x8F, modRM(0b10, 0, reg));
    else
        bytes_emitted += emit_bytes(REX_B, 0x8F, modRM(0b10, 0, reg - 8));

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

    bytes_emitted += emit_bytes(rex, 0x81, modRM(0b11, 5, reg));
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

    return emit_bytes(rex, 0x29, modRM(0b11, src, dest));
}



// add reg64, imm32
size_t emit_add_reg_imm32(emit_ctx_t * ctx, int reg, int32_t imm32)
{
    asm_emit("add %s, %d\n", reg_names[reg], imm32);

    uint8_t rex = REX_W;

    check_dst_reg(reg, rex);

    size_t bytes_emitted = 0;

    bytes_emitted += emit_bytes(rex, 0x81, modRM(0b11, 0, reg));
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

    return emit_bytes(rex, 0x01, modRM(0b11, src, dest));
}
/**************************************************/


/******************** IDIV, IMUL ********************/
// idiv reg64
size_t emit_idiv_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("idiv %s\n", reg_names[reg]);

    uint8_t rex = REX_W;
    check_dst_reg(reg, rex);

    return emit_bytes(rex, 0xF7, modRM(0b11, 7, reg));
}


// imul reg64
size_t emit_imul_reg(emit_ctx_t * ctx, int reg)
{
    asm_emit("imul %s\n", reg_names[reg]);

    uint8_t rex = REX_W;
    check_dst_reg(reg, rex);

    return emit_bytes(rex, 0xF7, modRM(0b11, 5, reg));
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

    return emit_bytes(rex, 0x89, modRM(0b11, src, dst));
}


// mov QWORD[reg64 + imm32], reg64
size_t emit_mov_mem_reg(emit_ctx_t * ctx, int dst, int32_t imm32, int src)
{
    asm_emit("mov QWORD[%s+(%d)], %s\n", reg_names[dst], imm32, reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    size_t emitted_bytes = 0;

    emitted_bytes += emit_bytes(rex, 0x89, modRM(0b10, src, dst));
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

    emitted_bytes += emit_bytes(rex, 0xB8 + reg);
    emitted_bytes += emit_imm64(imm64);

    return emitted_bytes;
}
/*********************************************/


/****************** JMP, JZ, CALL ******************/
// jmp imm32 (near)
size_t emit_jmp_rel32(emit_ctx_t * ctx, int32_t rel32)
{
    asm_emit("jmp $ + (%d)\n", rel32);

    size_t emitted_bytes = 0;
    emitted_bytes += emit_bytes(0xE9);
    emitted_bytes += emit_imm32(rel32);

    return emitted_bytes;
}

// jz imm32 (near)
size_t emit_jz_rel32(emit_ctx_t * ctx, int32_t rel32)
{
    asm_emit("jz $ + (%d)\n", rel32);

    size_t emitted_bytes = 0;
    emitted_bytes += emit_bytes(0x0F, 0x84);
    emitted_bytes += emit_imm32(rel32);

    return emitted_bytes;
}


// call imm32 (near)
size_t emit_call_rel32(emit_ctx_t * ctx, int32_t rel32)
{
    asm_emit("call $ + (%d)\n", rel32);

    size_t emitted_bytes = 0;
    emitted_bytes += emit_bytes(0xE8);
    emitted_bytes += emit_imm32(rel32);

    return emitted_bytes;
}
/***************************************************/


// xor reg64, reg64
size_t emit_xor_reg_reg(emit_ctx_t * ctx, int dst, int src)
{
    asm_emit("xor %s, %s\n", reg_names[dst], reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, 0x31, modRM(0b11, src, dst));
}


// test reg64, reg64
size_t emit_test_reg_reg(emit_ctx_t * ctx, int dst, int src)
{
    asm_emit("test %s, %s\n", reg_names[dst], reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, 0x85, modRM(0b11, src, dst));
}

// cmp reg64, reg64
size_t emit_cmp_reg_reg(emit_ctx_t * ctx, int dst, int src)
{
    asm_emit("cmp %s, %s\n", reg_names[dst], reg_names[src]);

    uint8_t rex = REX_W;
    check_dst_reg(dst, rex);
    check_src_reg(src, rex);

    return emit_bytes(rex, 0x39, modRM(0b11, src, dst));
}

// setcc
size_t emit_setcc_reg8(emit_ctx_t * ctx, enum cmp_emit_num cmp_num, int reg)
{
    size_t emitted_bytes = 0;

    if (reg >= R_RSP)
        emitted_bytes += emit_bytes(REX_CODE);

    emitted_bytes += emit_bytes(0x0F);
    switch (cmp_num){
        case EMIT_GREATER:
            asm_emit("setg %s\n", reg8_names[reg]);
            emitted_bytes += emit_bytes(0x9F);
            break;

        case EMIT_LESS:
            asm_emit("setl %s\n", reg8_names[reg]);
            emitted_bytes += emit_bytes(0x9C);
            break;

        case EMIT_GREATER_EQ:
            asm_emit("setge %s\n", reg8_names[reg]);
            emitted_bytes += emit_bytes(0x9D);
            break;

        case EMIT_LESS_EQ:
            asm_emit("setle %s\n", reg8_names[reg]);
            emitted_bytes += emit_bytes(0x9E);
            break;

        case EMIT_EQUAL:
            asm_emit("sete %s\n", reg8_names[reg]);
            emitted_bytes += emit_bytes(0x94);
            break;

        case EMIT_N_EQUAL:
            asm_emit("setne %s\n", reg8_names[reg]);
            emitted_bytes += emit_bytes(0x95);
            break;
    }

    emitted_bytes += emit_bytes(modRM(0b11, 0, reg));

    return emitted_bytes;
}


// cvtsd2si reg64, xmmN
// currently not supports r8+
size_t emit_cvtsd2si_reg_xmm(emit_ctx_t * ctx, int reg64, int xmm)
{
    assert(reg64 <= R_RDI);
    asm_emit("cvtsd2si %s, %s\n", reg_names[reg64], xmm_names[xmm]);

    return emit_bytes(0xF2, REX_W, 0x0F, 0x2D, modRM(0b11, reg64, xmm));
}


// cvtsi2sd xmmN, reg64
// currently not supports r8+
size_t emit_cvtsi2sd_xmm_reg(emit_ctx_t * ctx, int xmm, int reg64)
{
    assert(reg64 <= R_RDI);
    asm_emit("cvtsd2si %s, %s\n",  xmm_names[xmm], reg_names[reg64]);

    return emit_bytes(0xF2, REX_W, 0x0F, 0x2A, modRM(0b11, xmm, reg64));
}


// sqrtsd xmm, xmm
size_t emit_sqrtsd_xmm_xmm(emit_ctx_t * ctx, int xmm_dst, int xmm_src)
{
    asm_emit("sqrtsd %s, %s\n", xmm_names[xmm_dst], xmm_names[xmm_src]);

    return emit_bytes(0xF2, 0x0F, 0x51, modRM(0b11, xmm_dst, xmm_src));
}


// ret
size_t emit_ret(emit_ctx_t * ctx)
{
    asm_emit("ret\n");
    return emit_bytes(0xC3);
}

// cqo
size_t emit_cqo(emit_ctx_t * ctx)
{
    asm_emit("cqo\n");

    return emit_bytes(0x48, 0x99);
}

// syscall
size_t emit_syscall(emit_ctx_t * ctx)
{
    asm_emit("syscall\n");

    return emit_bytes(0x0F, 0x05);
}


