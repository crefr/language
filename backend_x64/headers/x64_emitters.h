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

enum xmm_regs {
    XMM0 = 0,
    XMM1 = 1,
    XMM2 = 2,
    XMM3 = 3,
    XMM4 = 4,
    XMM5 = 5,
    XMM6 = 6,
    XMM7 = 7,
};


const char * const reg_names[16] = {
    "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
    "R8" , "R9" , "R10", "R11", "R12", "R13", "R14", "R15"
};

const char * const reg8_names[8] = {
    "AL" , "CL" , "DL" , "BL" , "SPL", "BPL", "SIL", "DIL"
};

const char * const xmm_names[8] = {
    "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7"
};


enum cmp_emit_num {
    EMIT_GREATER    = 0,
    EMIT_LESS       = 1,
    EMIT_GREATER_EQ = 2,
    EMIT_LESS_EQ    = 3,
    EMIT_EQUAL      = 4,
    EMIT_N_EQUAL    = 5,
};


/******************** PUSH ********************/
// push reg64
size_t emit_push_reg(emit_ctx_t * ctx, int reg);

// push imm32
size_t emit_push_imm32(emit_ctx_t * ctx, int32_t imm32);

// push QWORD[reg + imm32]
size_t emit_push_mem(emit_ctx_t * ctx, int reg, int32_t imm32);
/**********************************************/


/******************** POP ********************/
// pop reg64
size_t emit_pop_reg(emit_ctx_t * ctx, int reg);

// pop QWORD[reg + imm32]
size_t emit_pop_mem(emit_ctx_t * ctx, int reg, int32_t imm32);
/*********************************************/


/******************** SUB, ADD ********************/
// sub reg64, imm32
size_t emit_sub_reg_imm32(emit_ctx_t * ctx, int reg, int32_t imm32);

// sub reg64, reg64
size_t emit_sub_reg_reg(emit_ctx_t * ctx, int dest, int src);

// add reg64, imm32
size_t emit_add_reg_imm32(emit_ctx_t * ctx, int reg, int32_t imm32);

// sub reg64, reg64
size_t emit_add_reg_reg(emit_ctx_t * ctx, int dest, int src);
/**************************************************/


/******************** IDIV, IMUL ********************/
// idiv reg64
size_t emit_idiv_reg(emit_ctx_t * ctx, int reg);

// imul reg64
size_t emit_imul_reg(emit_ctx_t * ctx, int reg);
/****************************************************/


/******************** MOV ********************/
// mov reg64, reg64
size_t emit_mov_reg_reg(emit_ctx_t * ctx, int dst, int src);

// mov QWORD[reg64 + imm32], reg64
size_t emit_mov_mem_reg(emit_ctx_t * ctx, int dst, int32_t imm32, int src);

// mov reg64, imm64
size_t emit_mov_reg_imm(emit_ctx_t * ctx, int reg, int64_t imm64);
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
size_t emit_test_reg_reg(emit_ctx_t * cxt, int dst, int src);

// xor reg64, reg64
size_t emit_xor_reg_reg(emit_ctx_t * ctx, int dst, int src);

// cmp reg64, reg64
size_t emit_cmp_reg_reg(emit_ctx_t * ctx, int dst, int src);

// setcc reg8
size_t emit_setcc_reg8(emit_ctx_t * ctx, enum cmp_emit_num cmp_num, int reg);

// cvtsd2si reg64, xmmN
// currently not supports r8+
size_t emit_cvtsd2si_reg_xmm(emit_ctx_t * ctx, int reg64, int xmm);

// cvtsi2sd xmmN, reg64
// currently not supports r8+
size_t emit_cvtsi2sd_xmm_reg(emit_ctx_t * ctx, int xmm, int reg64);

// sqrtsd xmm, xmm
size_t emit_sqrtsd_xmm_xmm(emit_ctx_t * ctx, int xmm_dst, int xmm_src);

// ret
size_t emit_ret(emit_ctx_t * ctx);

// cqo
size_t emit_cqo(emit_ctx_t * ctx);

// syscall
size_t emit_syscall(emit_ctx_t * ctx);


#endif
