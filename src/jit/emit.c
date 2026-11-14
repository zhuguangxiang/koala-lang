/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "emit.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Encoding notation (Intel SDM): REX.W=48 (64-bit operand); /r=ModR/M reg operand;
 * /0../7=ModR/M reg field is an opcode extension; ib/id/io=imm8/imm32/imm64;
 * rel32=32-bit relative offset; modrm(mod,reg,rm): mod=3 reg-direct, 1 disp8, 2 disp32. */

/* Initialize a growable code buffer (256-byte initial capacity). */
void emit_init(EmitBuf *b)
{
    b->cap = 256;
    b->len = 0;
    b->data = malloc(b->cap);
}

/* Release the buffer and reset it to empty. */
void emit_free(EmitBuf *b)
{
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* Current write offset = number of bytes emitted so far. */
size_t emit_pos(EmitBuf *b) { return b->len; }

/* Ensure room for `extra` more bytes, doubling capacity until it fits. */
static void emit_reserve(EmitBuf *b, size_t extra)
{
    if (b->len + extra <= b->cap) return;
    while (b->len + extra > b->cap) b->cap *= 2;
    b->data = realloc(b->data, b->cap);
}

/* Append one raw byte. */
void emit_u8(EmitBuf *b, uint8_t v)
{
    emit_reserve(b, 1);
    b->data[b->len++] = v;
}

/* Append a little-endian uint32. */
void emit_u32(EmitBuf *b, uint32_t v)
{
    emit_reserve(b, 4);
    memcpy(b->data + b->len, &v, 4);
    b->len += 4;
}

/* Append a little-endian int32. */
void emit_i32(EmitBuf *b, int32_t v) { emit_u32(b, (uint32_t)v); }

/* Overwrite a little-endian int32 at absolute offset `off` (used for rel32 fixups). */
void emit_patch_i32(EmitBuf *b, size_t off, int32_t v)
{
    memcpy(b->data + off, &v, 4);
}

/* REX prefix 0100WRXB = 0x40|W<<3|R<<2|X<<1|B: W=64-bit, R/X/B extend reg/index/base. */
static void emit_rex(EmitBuf *b, int w, int r, int x, int bb)
{
    uint8_t rex = 0x40 | (w << 3) | ((r & 1) << 2) | ((x & 1) << 1) | (bb & 1);
    /* Only emit when it carries information (W set or an extension bit set). */
    if (rex != 0x40) emit_u8(b, rex);
}

/* ModR/M byte = mod(2)<<6 | reg(3)<<3 | rm(3). */
static void emit_modrm(EmitBuf *b, int mod, int reg, int rm)
{
    emit_u8(b, (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm & 7)));
}

/*
 * ModR/M + displacement for a [base + disp] memory operand.
 * base must not be RSP/R12 (those need a SIB byte); the JIT only uses RBX/RBP.
 * A displacement is always emitted (mod=01 for disp8, mod=10 for disp32) so
 * that RBP-relative addressing never collides with the mod=00 disp32 form.
 */
static void emit_modrm_mem(EmitBuf *b, int reg, int base, int32_t disp)
{
    if (disp >= -128 && disp <= 127) {
        emit_modrm(b, 0x1, reg, base);
        emit_u8(b, (uint8_t)(int8_t)disp);
    } else {
        emit_modrm(b, 0x2, reg, base);
        emit_i32(b, disp);
    }
}

/* mov reg, [base+disp]  (reg = mem64): REX.W 8B /r, modrm(reg,base) + disp8|disp32. */
void emit_mov_reg_mem(EmitBuf *b, int reg, int base, int32_t disp)
{
    emit_rex(b, 1, reg >> 3, 0, base >> 3);
    emit_u8(b, 0x8B);
    emit_modrm_mem(b, reg, base, disp);
}

/* mov [base+disp], reg  (mem64 = reg): REX.W 89 /r, modrm(reg,base) + disp8|disp32. */
void emit_mov_mem_reg(EmitBuf *b, int base, int32_t disp, int reg)
{
    emit_rex(b, 1, reg >> 3, 0, base >> 3);
    emit_u8(b, 0x89);
    emit_modrm_mem(b, reg, base, disp);
}

/* mov qword [base+disp], imm32  (sign-extended to 64): REX.W C7 /0 + id. */
void emit_mov_mem_imm32(EmitBuf *b, int base, int32_t disp, int32_t imm)
{
    emit_rex(b, 1, 0, 0, base >> 3);
    emit_u8(b, 0xC7);
    emit_modrm_mem(b, 0, base, disp);
    emit_i32(b, imm);
}

/* mov dst, src  (dst = src): REX.W 89 /r, modrm(11,src,dst). */
void emit_mov_reg_reg(EmitBuf *b, int dst, int src)
{
    emit_rex(b, 1, src >> 3, 0, dst >> 3);
    emit_u8(b, 0x89);
    emit_modrm(b, 0x3, src, dst);
}

/* mov reg, imm32  (sign-extended to 64): REX.W C7 /0, modrm(11,0,reg) + id. */
void emit_mov_reg_imm32(EmitBuf *b, int reg, int32_t imm)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xC7);
    emit_modrm(b, 0x3, 0, reg);
    emit_i32(b, imm);
}

/* movabs reg, imm64: REX.W (B8+r) + io (full 8-byte immediate). */
void emit_mov_reg_imm64(EmitBuf *b, int reg, uint64_t imm)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, (uint8_t)(0xB8 + (reg & 7)));
    emit_reserve(b, 8);
    memcpy(b->data + b->len, &imm, 8);
    b->len += 8;
}

/* reg/mem and reg/reg ALU share the opcode map 0x03 + op*8 (OP r64, r/m64). */
static uint8_t alu_opcode(int op) { return (uint8_t)(0x03 + op * 8); }

/* reg = reg OP [base+disp]: REX.W (03+op*8) /r, modrm(reg,base) + disp8|disp32. */
void emit_alu_reg_mem(EmitBuf *b, int op, int reg, int base, int32_t disp)
{
    emit_rex(b, 1, reg >> 3, 0, base >> 3);
    emit_u8(b, alu_opcode(op));
    emit_modrm_mem(b, reg, base, disp);
}

/* ALU reg,reg: dst = dst OP src. The destination is `dst` (the last argument).
 * REX.W (03+op*8) /r, modrm(11,dst,src): the opcode is "OP reg, r/m64", so the
 * ModR/M reg field (dst) is written and r/m (src) is the other operand. */
void emit_alu_reg_reg(EmitBuf *b, int op, int src, int dst)
{
    emit_rex(b, 1, dst >> 3, 0, src >> 3);
    emit_u8(b, alu_opcode(op));
    emit_modrm(b, 0x3, dst, src);
}

/* reg = reg OP imm32: REX.W 81 /op, modrm(11,op,reg) + id. */
void emit_alu_reg_imm32(EmitBuf *b, int op, int reg, int32_t imm)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0x81);
    emit_modrm(b, 0x3, op, reg);
    emit_i32(b, imm);
}

/* [base+disp] = [base+disp] OP imm32: REX.W 81 /op, modrm(op,base) + disp + id. */
void emit_alu_mem_imm32(EmitBuf *b, int op, int base, int32_t disp, int32_t imm)
{
    emit_rex(b, 1, 0, 0, base >> 3);
    emit_u8(b, 0x81);
    emit_modrm_mem(b, op, base, disp);
    emit_i32(b, imm);
}

/* reg = reg * [base+disp]  (signed): REX.W 0F AF /r, modrm(reg,base) + disp8|disp32. */
void emit_imul_reg_mem(EmitBuf *b, int reg, int base, int32_t disp)
{
    emit_rex(b, 1, reg >> 3, 0, base >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0xAF);
    emit_modrm_mem(b, reg, base, disp);
}

/* dst = dst * src  (signed): REX.W 0F AF /r, modrm(11,dst,src). */
void emit_imul_reg_reg(EmitBuf *b, int dst, int src)
{
    emit_rex(b, 1, dst >> 3, 0, src >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0xAF);
    emit_modrm(b, 0x3, dst, src);
}

/* cqo -- sign-extend rax into rdx:rax (prepare dividend for idiv): 48 99. */
void emit_cqo(EmitBuf *b)
{
    emit_u8(b, 0x48);
    emit_u8(b, 0x99);
}

/* idiv qword [base+disp] -- rdx:rax / mem, quotient->rax remainder->rdx: REX.W F7 /7. */
void emit_idiv_mem(EmitBuf *b, int base, int32_t disp)
{
    emit_rex(b, 1, 0, 0, base >> 3);
    emit_u8(b, 0xF7);
    emit_modrm_mem(b, 7, base, disp);
}

/* idiv reg -- rdx:rax / reg, quotient->rax remainder->rdx: REX.W F7 /7, modrm(11,7,reg). */
void emit_idiv_reg(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xF7);
    emit_modrm(b, 0x3, 7, reg);
}

/* div qword [base+disp] -- unsigned rdx:rax / mem: REX.W F7 /6. */
void emit_div_mem(EmitBuf *b, int base, int32_t disp)
{
    emit_rex(b, 1, 0, 0, base >> 3);
    emit_u8(b, 0xF7);
    emit_modrm_mem(b, 6, base, disp);
}

/* div reg -- unsigned rdx:rax / reg: REX.W F7 /6, modrm(11,6,reg). */
void emit_div_reg(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xF7);
    emit_modrm(b, 0x3, 6, reg);
}

/* shl reg, imm8: REX.W C1 /4 ib, modrm(11,4,reg). */
void emit_shl_reg_imm8(EmitBuf *b, int reg, uint8_t imm)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xC1);
    emit_modrm(b, 0x3, 4, reg);
    emit_u8(b, imm);
}

/* sar reg, imm8 (arithmetic): REX.W C1 /7 ib, modrm(11,7,reg). */
void emit_sar_reg_imm8(EmitBuf *b, int reg, uint8_t imm)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xC1);
    emit_modrm(b, 0x3, 7, reg);
    emit_u8(b, imm);
}

/* shl reg, cl: REX.W D3 /4, modrm(11,4,reg). */
void emit_shl_reg_cl(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xD3);
    emit_modrm(b, 0x3, 4, reg);
}

/* sar reg, cl (arithmetic): REX.W D3 /7, modrm(11,7,reg). */
void emit_sar_reg_cl(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xD3);
    emit_modrm(b, 0x3, 7, reg);
}

/* shr reg, imm8 (logical): REX.W C1 /5 ib, modrm(11,5,reg). */
void emit_shr_reg_imm8(EmitBuf *b, int reg, uint8_t imm)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xC1);
    emit_modrm(b, 0x3, 5, reg);
    emit_u8(b, imm);
}

/* shr reg, cl (logical): REX.W D3 /5, modrm(11,5,reg). */
void emit_shr_reg_cl(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xD3);
    emit_modrm(b, 0x3, 5, reg);
}

/* neg reg  (reg = -reg): REX.W F7 /3, modrm(11,3,reg). */
void emit_neg_reg(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xF7);
    emit_modrm(b, 0x3, 3, reg);
}

/* not reg  (reg = ~reg): REX.W F7 /2, modrm(11,2,reg). */
void emit_not_reg(EmitBuf *b, int reg)
{
    emit_rex(b, 1, 0, 0, reg >> 3);
    emit_u8(b, 0xF7);
    emit_modrm(b, 0x3, 2, reg);
}

/* reg = (flags satisfy cc) ? 1 : 0, emitted as two instructions: */
void emit_setcc_reg(EmitBuf *b, int cc, int reg)
{
    /* setcc al: 0F (90+cc) /0, modrm(11,0,RAX). */
    emit_u8(b, 0x0F);
    emit_u8(b, (uint8_t)(0x90 + cc));
    emit_modrm(b, 0x3, 0, RAX);
    /* movzx reg, al: REX.W 0F B6 /r, modrm(11,reg,RAX). */
    emit_rex(b, 1, reg >> 3, 0, 0);
    emit_u8(b, 0x0F);
    emit_u8(b, 0xB6);
    emit_modrm(b, 0x3, reg, RAX);
}

/* jmp rel32: E9 + rel32 (relative to next insn); patched later via emit_patch_i32. */
void emit_jmp_rel32(EmitBuf *b, int32_t rel)
{
    emit_u8(b, 0xE9);
    emit_i32(b, rel);
}

/* jcc rel32: 0F (80+cc) + rel32 (relative to next insn). */
void emit_jcc_rel32(EmitBuf *b, int cc, int32_t rel)
{
    emit_u8(b, 0x0F);
    emit_u8(b, (uint8_t)(0x80 + cc));
    emit_i32(b, rel);
}

/* call rel32 -- direct near call, pushes rip: E8 + rel32 (relative to next insn). */
void emit_call_rel32(EmitBuf *b, int32_t rel)
{
    emit_u8(b, 0xE8);
    emit_i32(b, rel);
}

/* call reg -- indirect near call, pushes rip: [REX.B 41 when reg>=8] FF /2, modrm(11,2,reg). */
void emit_call_reg(EmitBuf *b, int reg)
{
    if (reg >= 8) emit_u8(b, 0x41);
    emit_u8(b, 0xFF);
    emit_modrm(b, 0x3, 2, reg);
}

/* ret -- near return, pops rip: C3. */
void emit_ret(EmitBuf *b) { emit_u8(b, 0xC3); }

/* push reg: [REX.B 41 when reg>=8] + (50+r). */
void emit_push_reg(EmitBuf *b, int reg)
{
    if (reg >= 8) emit_u8(b, 0x41);
    emit_u8(b, (uint8_t)(0x50 + (reg & 7)));
}

/* pop reg: [REX.B 41 when reg>=8] + (58+r). */
void emit_pop_reg(EmitBuf *b, int reg)
{
    if (reg >= 8) emit_u8(b, 0x41);
    emit_u8(b, (uint8_t)(0x58 + (reg & 7)));
}

/* sub rsp, imm32: 48 81 /5 id, modrm(11,5,RSP). */
void emit_sub_rsp_imm32(EmitBuf *b, int32_t imm)
{
    emit_u8(b, 0x48);
    emit_u8(b, 0x81);
    emit_modrm(b, 0x3, 5, RSP);
    emit_i32(b, imm);
}

/* add rsp, imm32: 48 81 /0 id, modrm(11,0,RSP). */
void emit_add_rsp_imm32(EmitBuf *b, int32_t imm)
{
    emit_u8(b, 0x48);
    emit_u8(b, 0x81);
    emit_modrm(b, 0x3, 0, RSP);
    emit_i32(b, imm);
}

/* ---- SSE2 scalar double (mandatory F2 prefix, then [REX] 0F xx) ---- */

/* movsd xmm, [base+disp]: F2 [REX] 0F 10 /r. */
void emit_movsd_reg_mem(EmitBuf *b, int xmm, int base, int32_t disp)
{
    emit_u8(b, 0xF2);
    emit_rex(b, 0, xmm >> 3, 0, base >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x10);
    emit_modrm_mem(b, xmm, base, disp);
}

/* movsd [base+disp], xmm: F2 [REX] 0F 11 /r. */
void emit_movsd_mem_reg(EmitBuf *b, int base, int32_t disp, int xmm)
{
    emit_u8(b, 0xF2);
    emit_rex(b, 0, xmm >> 3, 0, base >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x11);
    emit_modrm_mem(b, xmm, base, disp);
}

/* addsd/subsd/mulsd/divsd xmm, [base+disp]: F2 [REX] 0F op /r. */
void emit_sse_reg_mem(EmitBuf *b, int op, int xmm, int base, int32_t disp)
{
    emit_u8(b, 0xF2);
    emit_rex(b, 0, xmm >> 3, 0, base >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, (uint8_t)op);
    emit_modrm_mem(b, xmm, base, disp);
}

/* ucomisd xmm, [base+disp]: 66 [REX] 0F 2E /r. */
void emit_ucomisd_reg_mem(EmitBuf *b, int xmm, int base, int32_t disp)
{
    emit_u8(b, 0x66);
    emit_rex(b, 0, xmm >> 3, 0, base >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x2E);
    emit_modrm_mem(b, xmm, base, disp);
}

/* addsd/subsd/mulsd/divsd dst, src (xmm, xmm): F2 [REX] 0F op /r, mod=11. */
void emit_sse_reg_reg(EmitBuf *b, int op, int dst, int src)
{
    emit_u8(b, 0xF2);
    emit_rex(b, 0, dst >> 3, 0, src >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, (uint8_t)op);
    emit_modrm(b, 0x3, dst, src);
}

/* ucomisd a, c (xmm, xmm): 66 [REX] 0F 2E /r, mod=11. */
void emit_ucomisd_reg_reg(EmitBuf *b, int a, int c)
{
    emit_u8(b, 0x66);
    emit_rex(b, 0, a >> 3, 0, c >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x2E);
    emit_modrm(b, 0x3, a, c);
}

/* movaps dst, src (xmm, xmm): [REX] 0F 28 /r. A full-register copy, so no false
 * dependency on dst's upper half (unlike movsd xmm, xmm). */
void emit_movaps_reg_reg(EmitBuf *b, int dst, int src)
{
    emit_rex(b, 0, dst >> 3, 0, src >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x28);
    emit_modrm(b, 0x3, dst, src);
}

/* movq xmm, r64: 66 REX.W 0F 6E /r (reg = xmm, rm = gpr). */
void emit_movq_xmm_gpr(EmitBuf *b, int xmm, int gpr)
{
    emit_u8(b, 0x66);
    emit_rex(b, 1, xmm >> 3, 0, gpr >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x6E);
    emit_modrm(b, 0x3, xmm, gpr);
}

/* movq r64, xmm: 66 REX.W 0F 7E /r (reg = xmm, rm = gpr). */
void emit_movq_gpr_xmm(EmitBuf *b, int gpr, int xmm)
{
    emit_u8(b, 0x66);
    emit_rex(b, 1, xmm >> 3, 0, gpr >> 3);
    emit_u8(b, 0x0F);
    emit_u8(b, 0x7E);
    emit_modrm(b, 0x3, xmm, gpr);
}

/* ud2: 0F 0B (guaranteed #UD trap). */
void emit_ud2(EmitBuf *b)
{
    emit_u8(b, 0x0F);
    emit_u8(b, 0x0B);
}

#ifdef __cplusplus
}
#endif