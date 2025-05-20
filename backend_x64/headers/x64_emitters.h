#ifndef X64_EMITTERS_INCLUDED
#define X64_EMITTERS_INCLUDED

#include <stdbool.h>

typedef struct {
    FILE * bin_file;
    FILE * asm_file;

    bool emitting;
} emit_ctx_t;

enum regs {
    R_RAX = 0,
    R_RCX = 1,
    R_RDX = 2,
    R_RBX = 3,
    R_RSP = 4,
    R_RBP = 5,
    R_RSI = 6,
    R_RDI = 7,

    R_R8  = R_RAX + 8,
    R_R9  = R_RCX + 8,
    R_R10 = R_RDX + 8,
    R_R11 = R_RBX + 8,
    R_R12 = R_RSP + 8,
    R_R13 = R_RBP + 8,
    R_R14 = R_RSI + 8,
    R_R15 = R_RDI + 8
};


const char * reg_names[16] = {
    {R_RAX, "RAX"},
    {R_RDX, "RDX"},
    {R_RCX, "RCX"},
    {R_RBX, "RBX"},
    {R_RSP, "RSP"},
    {R_RBP, "RBP"},
    {R_RSI, "RSI"},
    {R_RDI, "RDI"},

    {R_R8,  "R8" },
    {R_R9,  "R9" },
    {R_R10, "R10"},
    {R_R11, "R11"},
    {R_R12, "R12"},
    {R_R13, "R13"},
    {R_R14, "R14"},
    {R_R15, "R15"}
};

/******************** PUSH ********************/
// push reg64
size_t emit_push_reg(emit_ctx_t * ctx, enum regs reg);

// push imm32
size_t emit_push_imm32(emit_ctx_t * ctx, int32_t imm32);

// push QWORD[reg + imm32]
size_t emit_push_mem(emit_ctx_t * ctx, enum regs reg, int32_t imm32);
/**********************************************/


/******************** POP ********************/
// pop reg64
size_t emit_pop_reg(emit_ctx_t * ctx, enum regs reg);

// pop QWORD[reg + imm32]
size_t emit_pop_mem(emit_ctx_t * ctx, enum regs reg, int32_t imm32);
/*********************************************/


/******************** SUB, ADD ********************/
// sub reg64, imm32
size_t emit_sub_reg_imm32(emit_ctx_t * ctx, enum regs reg, int32_t imm32);

// sub reg64, reg64
size_t emit_sub_reg_reg(emit_ctx_t * ctx, enum regs dest, enum regs src);

// add reg64, imm32
size_t emit_add_reg_imm32(emit_ctx_t * ctx, enum regs reg, int32_t imm32);

// sub reg64, reg64
size_t emit_add_reg_reg(emit_ctx_t * ctx, enum regs dest, enum regs src);
/**************************************************/


/******************** IDIV, IMUL ********************/
// idiv reg64
size_t emit_idiv_reg(emit_ctx_t * ctx, enum regs reg);

// imul reg64
size_t emit_imul_reg(emit_ctx_t * ctx, enum regs reg);
/****************************************************/


/******************** MOV ********************/
// mov reg64, reg64
size_t emit_mov_reg_reg(emit_ctx_t * ctx, enum regs dst, enum reds, src);

// mov QWORD[reg64 + imm32], reg64
size_t emit_mov_mem_reg(emit_ctx_t * ctx, enum regs dst, int32_t imm32, enum regs src);
/*********************************************/


/****************** JMP, JZ, CALL ******************/
// jmp imm32 (near)
size_t emit_jmp_rel32(emit_ctx_t * ctx, int32_t rel32);

// jz imm32 (near)
size_t emit_jz_rel32(emit_ctx_t * ctx, int32_t rel32);

// call imm32 (near)
size_t emit_call_rel32(emit_ctx_t * ctx, int32_t rel32);
/***************************************************/


// test reg64, reg64
size_t emit_test_reg_reg(emit_ctx_t * cxt, enum regs dst, enum regs src);

// ret
size_t emit_ret(emit_ctx_t * ctx);

// cqo
size_t emit_cqo(emit_ctx_t * ctx);

#endif
