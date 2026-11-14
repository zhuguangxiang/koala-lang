/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_JIT_EMIT_H_
#define _KOALA_JIT_EMIT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* x86-64 general purpose registers (encoding order). */
enum {
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15,
};

/* Condition codes (low nibble of SETcc/Jcc). */
enum {
    CC_O = 0x0, CC_NO = 0x1, CC_B = 0x2, CC_AE = 0x3,
    CC_E = 0x4, CC_NE = 0x5, CC_BE = 0x6, CC_A = 0x7,
    CC_S = 0x8, CC_NS = 0x9, CC_P = 0xA, CC_NP = 0xB,
    CC_L = 0xC, CC_GE = 0xD, CC_LE = 0xE, CC_G = 0xF,
};

/* Growable byte buffer holding the emitted machine code. */
typedef struct _EmitBuf {
    uint8_t *data;
    size_t len;
    size_t cap;
} EmitBuf;

void emit_init(EmitBuf *b);
void emit_free(EmitBuf *b);
size_t emit_pos(EmitBuf *b);
void emit_u8(EmitBuf *b, uint8_t v);
void emit_u32(EmitBuf *b, uint32_t v);
void emit_i32(EmitBuf *b, int32_t v);
/* Patch a little-endian int32 at an absolute buffer offset. */
void emit_patch_i32(EmitBuf *b, size_t off, int32_t v);

/* Integer ALU ops shared by reg/reg, reg/mem and reg/imm forms. */
enum {
    ALU_ADD = 0, ALU_OR = 1, ALU_ADC = 2, ALU_SBB = 3,
    ALU_AND = 4, ALU_SUB = 5, ALU_XOR = 6, ALU_CMP = 7,
};

/* Data movement. base is a GPR; disp is a signed byte displacement. */
void emit_mov_reg_mem(EmitBuf *b, int reg, int base, int32_t disp);
void emit_mov_mem_reg(EmitBuf *b, int base, int32_t disp, int reg);
void emit_mov_mem_imm32(EmitBuf *b, int base, int32_t disp, int32_t imm);
void emit_mov_reg_reg(EmitBuf *b, int dst, int src);
void emit_mov_reg_imm32(EmitBuf *b, int reg, int32_t imm);
void emit_mov_reg_imm64(EmitBuf *b, int reg, uint64_t imm);

/* ALU. The reg/mem and reg/imm forms compute dst = dst OP src.
 * emit_alu_reg_reg likewise writes its last argument: dst = dst OP src. */
void emit_alu_reg_mem(EmitBuf *b, int op, int reg, int base, int32_t disp);
void emit_alu_reg_reg(EmitBuf *b, int op, int src, int dst);
void emit_alu_reg_imm32(EmitBuf *b, int op, int reg, int32_t imm);
void emit_alu_mem_imm32(EmitBuf *b, int op, int base, int32_t disp, int32_t imm);
void emit_imul_reg_mem(EmitBuf *b, int reg, int base, int32_t disp);
void emit_imul_reg_reg(EmitBuf *b, int dst, int src);

/* Signed division: cqo + idiv. Dividend in rax, quotient rax, remainder rdx. */
void emit_cqo(EmitBuf *b);
void emit_idiv_mem(EmitBuf *b, int base, int32_t disp);
void emit_idiv_reg(EmitBuf *b, int reg);

/* Unsigned division: xor edx,edx + div. Dividend in rax, quotient rax, remainder rdx. */
void emit_div_mem(EmitBuf *b, int base, int32_t disp);
void emit_div_reg(EmitBuf *b, int reg);

/* Shifts. */
void emit_shl_reg_imm8(EmitBuf *b, int reg, uint8_t imm);
void emit_sar_reg_imm8(EmitBuf *b, int reg, uint8_t imm);
void emit_shr_reg_imm8(EmitBuf *b, int reg, uint8_t imm);
void emit_shl_reg_cl(EmitBuf *b, int reg);
void emit_sar_reg_cl(EmitBuf *b, int reg);
void emit_shr_reg_cl(EmitBuf *b, int reg);

/* Unary. */
void emit_neg_reg(EmitBuf *b, int reg);
void emit_not_reg(EmitBuf *b, int reg);

/* setcc al ; movzx reg, al  -> reg = (flags satisfy cc) ? 1 : 0 */
void emit_setcc_reg(EmitBuf *b, int cc, int reg);

/* Control flow. rel32 placeholders are patched later via emit_patch_i32. */
void emit_jmp_rel32(EmitBuf *b, int32_t rel);
void emit_jcc_rel32(EmitBuf *b, int cc, int32_t rel);
void emit_call_rel32(EmitBuf *b, int32_t rel);
void emit_call_reg(EmitBuf *b, int reg);
void emit_ret(EmitBuf *b);

/* Stack / frame. */
void emit_push_reg(EmitBuf *b, int reg);
void emit_pop_reg(EmitBuf *b, int reg);
void emit_sub_rsp_imm32(EmitBuf *b, int32_t imm);
void emit_add_rsp_imm32(EmitBuf *b, int32_t imm);

/* XMM registers (encoding order). */
enum { XMM0 = 0, XMM1 = 1, XMM2 = 2, XMM3 = 3, XMM4 = 4, XMM5 = 5, XMM6 = 6, XMM7 = 7 };

/* SSE2 scalar-double opcode bytes (the 0F xx byte after the F2 prefix). */
enum { SSE_ADD = 0x58, SSE_MUL = 0x59, SSE_SUB = 0x5C, SSE_DIV = 0x5E };

/* SSE2 scalar double. movsd loads/stores the 64-bit payload; the arithmetic
 * forms compute xmm = xmm OP [base+disp]. */
void emit_movsd_reg_mem(EmitBuf *b, int xmm, int base, int32_t disp);
void emit_movsd_mem_reg(EmitBuf *b, int base, int32_t disp, int xmm);
void emit_sse_reg_mem(EmitBuf *b, int op, int xmm, int base, int32_t disp);
/* ucomisd xmm, [base+disp]: unordered compare; NaN sets ZF=PF=CF=1. */
void emit_ucomisd_reg_mem(EmitBuf *b, int xmm, int base, int32_t disp);
/* Register forms: dst = dst OP src; ucomisd a, c; full xmm copy; bit moves
 * between xmm and r64 (movq). */
void emit_sse_reg_reg(EmitBuf *b, int op, int dst, int src);
void emit_ucomisd_reg_reg(EmitBuf *b, int a, int c);
void emit_movaps_reg_reg(EmitBuf *b, int dst, int src);
void emit_movq_xmm_gpr(EmitBuf *b, int xmm, int gpr);
void emit_movq_gpr_xmm(EmitBuf *b, int gpr, int xmm);

/* ud2: trap (used by the KOALA_JIT_CHECK_CACHE self-check). */
void emit_ud2(EmitBuf *b);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_EMIT_H_ */