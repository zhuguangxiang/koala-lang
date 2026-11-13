/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Baseline x86-64 JIT (v1). See DESIGN.md for the full design.
 *
 * Strategy: Sparkplug-style baseline. Each whitelisted bytecode instruction is
 * lowered to a fixed sequence of machine instructions that mirrors the
 * interpreter's semantics on the TValue register file, minus dispatch, decode
 * and type checks. Registers stay in the caller's frame (cf->locals); RBX holds
 * the register-file base, so register i lives at [RBX + i*16] (tag) and
 * [RBX + i*16 + 8] (payload). No register allocation is performed.
 */

#include "jit/jit.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "emit.h"
#include "modobj.h"
#include "opcode_only.h"
#include "printer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- bytecode decoding (must match runtime/eval.c) ---- */

/* Runtime int<->int cast (do_cast.c): may abort in trap mode, never TAG_ERROR. */
void do_int_cast(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_float_cast(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_float_to_int(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_int_to_float(TValue *regs, int rd, int rs, int mode, int dst_ti);

#define I_OP(i)             ((i) >> 24)
#define I_VAL(i, shr, mask) (((i) >> (shr)) & ((1u << (mask)) - 1))

/* Sentinel ALU code for multiplication (imul has its own encoding). */
#define ALU_MUL 8

static inline int32_t sval(uint32_t val, int bits)
{
    int shift = 32 - bits;
    return ((int32_t)(val << shift)) >> shift;
}

/* TValue layout: { uintptr_t tag; union { int64_t ival; ... }; } = 16 bytes. */
#define TAG_DISP(r) ((int32_t)((r) * 16))
#define VAL_DISP(r) ((int32_t)((r) * 16 + 8))

/* A pending rel32 jump: patch_off is the buffer offset of the 4-byte disp. */
typedef struct _Fixup {
    size_t patch_off;
    int target_pc;
} Fixup;

/* Sentinel fixup target: the function's error epilogue, whose native offset
 * is only known after the whole body has been emitted. */
#define JIT_ERR_TARGET (-1)

typedef struct _JitCtx {
    EmitBuf buf;
    size_t *pc_map; /* pc -> native offset, size num_insns */
    Fixup *fixups;
    int nfixups;
    int num_insns;
    int failed;
    int fail_pc;        /* pc of the unsupported instruction (if failed) */
    uint32_t fail_insn; /* the offending instruction word */
} JitCtx;

/* KOALA_JIT_TRACE: 0/1 switch, default off (unset or "0" disables tracing). */
static int jit_trace(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT_TRACE");
        cached = (v && v[0] != '0');
    }
    return cached;
}

int kl_jit_enabled(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT");
        cached = !(v && v[0] == '0');
    }
    return cached;
}

uint32_t kl_jit_threshold(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = getenv("KOALA_JIT_THRESHOLD");
        int n = v ? atoi(v) : 1;
        cached = n < 1 ? 1 : n;
    }
    return (uint32_t)cached;
}

/* ---- frame setup ---- */

/*
 * Prologue (System V AMD64):
 *   rdi = regs (TValue*), rsi = out (TValue*)
 *   push rbp; mov rbp,rsp; push rbx; sub rsp,16
 *   mov [rbp-16], rsi   ; save out pointer
 *   mov rbx, rdi        ; register-file base
 * Only caller-saved scratch (rax/rcx/rdx) is used in the body, so RBX is the
 * single callee-saved register we must preserve.
 */
static void emit_prologue(JitCtx *j)
{
    EmitBuf *b = &j->buf;
    emit_push_reg(b, RBP);
    emit_mov_reg_reg(b, RBP, RSP);
    emit_push_reg(b, RBX);
    emit_sub_rsp_imm32(b, 16);
    emit_mov_mem_reg(b, RBP, -16, RSI);
    emit_mov_reg_reg(b, RBX, RDI);
}

/* Restore the frame and return. The result TValue must already be in *out. */
static void emit_epilogue(JitCtx *j)
{
    EmitBuf *b = &j->buf;
    emit_add_rsp_imm32(b, 16);
    emit_pop_reg(b, RBX);
    emit_pop_reg(b, RBP);
    emit_ret(b);
}

/* Load the out pointer into RCX. */
static void emit_load_out(JitCtx *j) { emit_mov_reg_mem(&j->buf, RCX, RBP, -16); }

/* ---- jump emission with fixups ---- */

/* cc < 0 => unconditional JMP. target_pc is an absolute bytecode pc. */
static void emit_jump(JitCtx *j, int cc, int target_pc)
{
    EmitBuf *b = &j->buf;
    if (cc < 0)
        emit_jmp_rel32(b, 0);
    else
        emit_jcc_rel32(b, cc, 0);
    j->fixups[j->nfixups].patch_off = emit_pos(b) - 4;
    j->fixups[j->nfixups].target_pc = target_pc;
    j->nfixups++;
}

/* ---- instruction helpers ---- */

static void emit_store_tag(JitCtx *j, int rd, int32_t tag)
{
    emit_mov_mem_imm32(&j->buf, RBX, TAG_DISP(rd), tag);
}

/* Store RAX as the payload of rd with an INT64 tag. */
static void emit_store_int64(JitCtx *j, int rd)
{
    emit_mov_mem_reg(&j->buf, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_INT64);
}

/* Store RAX as the payload of rd with an explicit tag (INT64 / UINT64). */
static void emit_store_tagged(JitCtx *j, int rd, int32_t tag)
{
    emit_mov_mem_reg(&j->buf, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, tag);
}

/* rd = rs ALU rt, result tagged `tag`. alu_op is an ALU_* code or ALU_MUL.
 * The payload op is identical for the INT and UINT families; only the result
 * tag differs (two's-complement add/sub/mul/bitwise are sign-agnostic). */
static void emit_alu_rrr(JitCtx *j, int alu_op, int rd, int rs, int rt, int32_t tag)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    if (alu_op == ALU_MUL)
        emit_imul_reg_mem(b, RAX, RBX, VAL_DISP(rt));
    else
        emit_alu_reg_mem(b, alu_op, RAX, RBX, VAL_DISP(rt));
    emit_store_tagged(j, rd, tag);
}

/* rd = rs ALU imm, result tagged `tag`. */
static void emit_alu_rrimm(JitCtx *j, int alu_op, int rd, int rs, int32_t imm,
                           int32_t tag)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    if (alu_op == ALU_MUL) {
        emit_mov_reg_imm32(b, RCX, imm);
        emit_imul_reg_reg(b, RAX, RCX);
    } else {
        emit_alu_reg_imm32(b, alu_op, RAX, imm);
    }
    emit_store_tagged(j, rd, tag);
}

/* rd = rs / rt (is_mod=0) or rs % rt (is_mod=1). Signed uses cqo+idiv; unsigned
 * uses xor edx,edx+div. Result tag INT64 / UINT64 accordingly. */
static void emit_div_rrr(JitCtx *j, int rd, int rs, int rt, int is_mod,
                         int is_unsigned)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    if (is_unsigned) {
        emit_alu_reg_reg(b, ALU_XOR, RDX, RDX);
        emit_div_mem(b, RBX, VAL_DISP(rt));
    } else {
        emit_cqo(b);
        emit_idiv_mem(b, RBX, VAL_DISP(rt));
    }
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), is_mod ? RDX : RAX);
    emit_store_tag(j, rd, is_unsigned ? TAG_UINT64 : TAG_INT64);
}

static void emit_div_rrimm(JitCtx *j, int rd, int rs, int32_t imm, int is_mod,
                           int is_unsigned)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_mov_reg_imm32(b, RCX, imm);
    if (is_unsigned) {
        emit_alu_reg_reg(b, ALU_XOR, RDX, RDX);
        emit_div_reg(b, RCX);
    } else {
        emit_cqo(b);
        emit_idiv_reg(b, RCX);
    }
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), is_mod ? RDX : RAX);
    emit_store_tag(j, rd, is_unsigned ? TAG_UINT64 : TAG_INT64);
}

/* rd = (rs CC rt) as a BOOL. */
static void emit_int_cmp_rrr(JitCtx *j, int cc, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_alu_reg_mem(b, ALU_CMP, RAX, RBX, VAL_DISP(rt));
    emit_setcc_reg(b, cc, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
}

static void emit_int_cmp_rrimm(JitCtx *j, int cc, int rd, int rs, int32_t imm)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_alu_reg_imm32(b, ALU_CMP, RAX, imm);
    emit_setcc_reg(b, cc, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
}

/* Fused compare-and-branch: if (rs CC rt) goto target_pc. */
static void emit_jmp_cmp_rrr(JitCtx *j, int cc, int rs, int rt, int target_pc)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_alu_reg_mem(b, ALU_CMP, RAX, RBX, VAL_DISP(rt));
    emit_jump(j, cc, target_pc);
}

static void emit_jmp_cmp_rrimm(JitCtx *j, int cc, int rs, int32_t imm, int target_pc)
{
    EmitBuf *b = &j->buf;
    emit_alu_mem_imm32(b, ALU_CMP, RBX, VAL_DISP(rs), imm);
    emit_jump(j, cc, target_pc);
}

/* Register-form shift (OP_INT/UINT_SHL/SHR). Unlike the _IMM forms, the count
 * is validated at run time: kl_jit_shift_check mirrors the interpreter's
 * kl_check_shift_count + SAVE_PC + trace, returning 0 on raise so the native
 * code branches to the error epilogue. shift_kind: 0=shl, 1=sar, 2=shr.
 *   rdi = &regs[rt] (the count TValue), esi = pc_abs */
static void emit_reg_shift(JitCtx *j, int rd, int rs, int rt, int shift_kind,
                           int32_t tag, int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, rt * 16);
    emit_mov_reg_imm32(b, RSI, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)kl_jit_shift_check);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    emit_alu_reg_imm32(b, ALU_CMP, RAX, 0);
    emit_jump(j, CC_E, JIT_ERR_TARGET);
    /* count is valid: rax = value, rcx = count, shift by cl */
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_mov_reg_mem(b, RCX, RBX, VAL_DISP(rt));
    if (shift_kind == 0)
        emit_shl_reg_cl(b, RAX);
    else if (shift_kind == 1)
        emit_sar_reg_cl(b, RAX);
    else
        emit_shr_reg_cl(b, RAX);
    emit_store_tagged(j, rd, tag);
}

/* Float comparison opcode -> kl_jit_float_cmp op code (0=EQ..5=GE). */
static int float_cmp_op(OpCode op)
{
    switch (op) {
        case OP_FLOAT_EQ:
        case OP_JMP_FLOAT_EQ:
            return 0;
        case OP_FLOAT_NE:
        case OP_JMP_FLOAT_NE:
            return 1;
        case OP_FLOAT_LT:
        case OP_JMP_FLOAT_LT:
            return 2;
        case OP_FLOAT_LE:
        case OP_JMP_FLOAT_LE:
            return 3;
        case OP_FLOAT_GT:
        case OP_JMP_FLOAT_GT:
            return 4;
        case OP_FLOAT_GE:
        case OP_JMP_FLOAT_GE:
            return 5;
        default:
            return -1;
    }
}

/* rd = rs OP rt (scalar double, SSE2), result tag FLOAT64. The payload lives in
 * the same 8-byte slot as ival; values spill to the TValue file between ops, so
 * XMM0 is safe scratch. */
static void emit_float_arith(JitCtx *j, int sse_op, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    emit_movsd_reg_mem(b, XMM0, RBX, VAL_DISP(rs));
    emit_sse_reg_mem(b, sse_op, XMM0, RBX, VAL_DISP(rt));
    emit_movsd_mem_reg(b, RBX, VAL_DISP(rd), XMM0);
    emit_store_tag(j, rd, TAG_FLOAT64);
}

/* rd = -rs (double). IEEE negation is a sign-bit flip, done on the raw bits with
 * GPRs (bit-exact with the interpreter's `-fval`, including NaN/zero). */
static void emit_float_neg(JitCtx *j, int rd, int rs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
    emit_mov_reg_imm64(b, RDX, (uint64_t)0x8000000000000000ULL);
    /* emit_alu_reg_reg writes its last argument: dst = dst OP src, so RAX = RAX ^ RDX */
    emit_alu_reg_reg(b, ALU_XOR, RDX, RAX);
    emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_FLOAT64);
}

/* rd = fmod(rs, rt): args in xmm0/xmm1, result in xmm0 (System V). */
static void emit_float_mod(JitCtx *j, int rd, int rs, int rt)
{
    EmitBuf *b = &j->buf;
    emit_movsd_reg_mem(b, XMM0, RBX, VAL_DISP(rs));
    emit_movsd_reg_mem(b, XMM1, RBX, VAL_DISP(rt));
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)fmod);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    emit_movsd_mem_reg(b, RBX, VAL_DISP(rd), XMM0);
    emit_store_tag(j, rd, TAG_FLOAT64);
}

/* Call kl_jit_float_cmp(rs, rt, cmp_op); result (0/1) left in rax. */
static void emit_float_cmp_call(JitCtx *j, int rs, int rt, int cmp_op)
{
    EmitBuf *b = &j->buf;
    emit_movsd_reg_mem(b, XMM0, RBX, VAL_DISP(rs));
    emit_movsd_reg_mem(b, XMM1, RBX, VAL_DISP(rt));
    emit_mov_reg_imm32(b, RDI, cmp_op);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)kl_jit_float_cmp);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
}

/* rd = (rs CC rt) as BOOL via the float-compare helper. */
static void emit_float_cmp_bool(JitCtx *j, int rd, int rs, int rt, int cmp_op)
{
    emit_float_cmp_call(j, rs, rt, cmp_op);
    emit_mov_mem_reg(&j->buf, RBX, VAL_DISP(rd), RAX);
    emit_store_tag(j, rd, TAG_BOOL);
}

/* Fused float compare-and-branch: if (rs CC rt) goto target_pc. */
static void emit_float_jmp_cmp(JitCtx *j, int rs, int rt, int cmp_op, int target_pc)
{
    emit_float_cmp_call(j, rs, rt, cmp_op);
    emit_alu_reg_imm32(&j->buf, ALU_CMP, RAX, 0);
    emit_jump(j, CC_NE, target_pc);
}

/* Cast helper call: fn(regs, rd, rs, mode, dst_ti). May abort (trap mode); the
 * helper writes regs[rd] directly, no TAG_ERROR. Used by int/float casts. */
static void emit_cast_call(JitCtx *j, int rd, int rs, int mode, int dst_ti, void *fn)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_mov_reg_imm32(b, RSI, rd);
    emit_mov_reg_imm32(b, RDX, rs);
    emit_mov_reg_imm32(b, RCX, mode);
    emit_mov_reg_imm32(b, R8, dst_ti);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)fn);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
}

/* Ordinary call (flag=0) through the runtime helper kl_jit_call_helper, which
 * mirrors the interpreter's OP_CALL: IS_CFUNC -> kl_call_cfunc, else kl_call_code
 * (which itself dispatches to native when the callee is JIT-compiled, interpreter
 * otherwise). By the VM sliding-window convention the outgoing args sit at
 * caller_regs + nlocals, which equals ks->stack_top at a native call site; the
 * result lands in caller_regs[rd].
 *   rdi = args = RBX + nlocals*16
 *   rsi = out  = RBX + rd*16 (NULL when rd == 0xFFF, a void call)
 *   rdx = fn   (resolved ENTRY(payload), embedded as imm64)
 *   rcx = nargs
 *   r8  = pc_abs (absolute bytecode pc of this OP_CALL, for cf->pc/traceback)
 * The helper returns the callee's TValue in rax:rdx; a TAG_ERROR result jumps
 * to the error epilogue, mirroring the interpreter's `goto error`.
 * RBX/RBP are callee-saved (prologue/epilogue preserve them); rax/rcx/rdx/rsi/rdi
 * are scratch. The body runs with rsp ≡ 8 (mod 16), so pad 8 to reach the 16-byte
 * alignment the ABI requires at a call site. The helper address is absolute
 * (imm64) and called indirectly, so the code is position-independent: no fixup. */
static void emit_helper_call(JitCtx *j, int rd, int nargs, int nlocals, Object *fn,
                             int pc_abs)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, nlocals * 16);
    if (rd == 0xFFF) {
        emit_mov_reg_imm32(b, RSI, 0);
    } else {
        emit_mov_reg_reg(b, RSI, RBX);
        emit_alu_reg_imm32(b, ALU_ADD, RSI, rd * 16);
    }
    emit_mov_reg_imm64(b, RDX, (uint64_t)(uintptr_t)fn);
    emit_mov_reg_imm32(b, RCX, nargs);
    emit_mov_reg_imm32(b, R8, pc_abs);
    emit_sub_rsp_imm32(b, 8);
    emit_mov_reg_imm64(b, RAX, (uint64_t)(uintptr_t)kl_jit_call_helper);
    emit_call_reg(b, RAX);
    emit_add_rsp_imm32(b, 8);
    /* rax = ret.tag: propagate errors like the interpreter's OP_CALL */
    emit_alu_reg_imm32(b, ALU_CMP, RAX, TAG_ERROR);
    emit_jump(j, CC_E, JIT_ERR_TARGET);
}

/* Direct self-recursive call: re-enter this function's native entry (offset 0)
 * with a plain near call, skipping kl_call_code's frame machinery entirely. Only
 * used when EVERY call in the function targets itself, so the VM's ks->stack_top
 * is never read during the recursion and need not be advanced here. The outgoing
 * args already sit at RBX + nlocals*16 (the sliding window); the callee prologue
 * rebuilds its own machine frame. The target is always offset 0, so the rel32 is
 * computed inline (no fixup): rel = 0 - (pos_of_E8 + 5).
 *   rdi = args = RBX + nlocals*16
 *   rsi = out  = RBX + rd*16 (NULL when rd == 0xFFF)
 * No TAG_ERROR check is needed afterwards: a self-only function contains no
 * helper calls, so the only failure mode is SIGFPE, which kills the process
 * exactly as in the interpreter. */
static void emit_self_call(JitCtx *j, int rd, int nlocals)
{
    EmitBuf *b = &j->buf;
    emit_mov_reg_reg(b, RDI, RBX);
    emit_alu_reg_imm32(b, ALU_ADD, RDI, nlocals * 16);
    if (rd == 0xFFF) {
        emit_mov_reg_imm32(b, RSI, 0);
    } else {
        emit_mov_reg_reg(b, RSI, RBX);
        emit_alu_reg_imm32(b, ALU_ADD, RSI, rd * 16);
    }
    emit_sub_rsp_imm32(b, 8);
    int32_t rel = -(int32_t)(emit_pos(b) + 5);
    emit_call_rel32(b, rel);
    emit_add_rsp_imm32(b, 8);
}

/* ---- opcode classification ---- */

/* Signed integer comparison opcode -> condition code (-1 if not a compare). */
static int cmp_cc(OpCode op)
{
    switch (op) {
        case OP_INT_EQ:
        case OP_INT_EQ_IMM:
        case OP_JMP_INT_EQ:
        case OP_JMP_INT_EQ_IMM:
            return CC_E;
        case OP_INT_NE:
        case OP_INT_NE_IMM:
        case OP_JMP_INT_NE:
        case OP_JMP_INT_NE_IMM:
            return CC_NE;
        case OP_INT_LT:
        case OP_INT_LT_IMM:
        case OP_JMP_INT_LT:
        case OP_JMP_INT_LT_IMM:
            return CC_L;
        case OP_INT_LE:
        case OP_INT_LE_IMM:
        case OP_JMP_INT_LE:
        case OP_JMP_INT_LE_IMM:
            return CC_LE;
        case OP_INT_GT:
        case OP_INT_GT_IMM:
        case OP_JMP_INT_GT:
        case OP_JMP_INT_GT_IMM:
            return CC_G;
        case OP_INT_GE:
        case OP_INT_GE_IMM:
        case OP_JMP_INT_GE:
        case OP_JMP_INT_GE_IMM:
            return CC_GE;
        /* unsigned comparisons (uint64 family); EQ/NE reg-form reuse OP_INT_EQ/NE */
        case OP_UINT_EQ_IMM:
            return CC_E;
        case OP_UINT_NE_IMM:
            return CC_NE;
        case OP_UINT_LT:
        case OP_UINT_LT_IMM:
        case OP_JMP_UINT_LT:
        case OP_JMP_UINT_LT_IMM:
            return CC_B;
        case OP_UINT_LE:
        case OP_UINT_LE_IMM:
        case OP_JMP_UINT_LE:
        case OP_JMP_UINT_LE_IMM:
            return CC_BE;
        case OP_UINT_GT:
        case OP_UINT_GT_IMM:
        case OP_JMP_UINT_GT:
        case OP_JMP_UINT_GT_IMM:
            return CC_A;
        case OP_UINT_GE:
        case OP_UINT_GE_IMM:
        case OP_JMP_UINT_GE:
        case OP_JMP_UINT_GE_IMM:
            return CC_AE;
        default:
            return -1;
    }
}

/* Integer arithmetic opcode -> ALU code (-1 if not arithmetic). */
static int arith_alu(OpCode op)
{
    switch (op) {
        case OP_INT_ADD:
        case OP_INT_ADD_IMM:
            return ALU_ADD;
        case OP_INT_SUB:
        case OP_INT_SUB_IMM:
            return ALU_SUB;
        case OP_INT_MUL:
        case OP_INT_MUL_IMM:
            return ALU_MUL;
        case OP_INT_AND:
        case OP_INT_AND_IMM:
            return ALU_AND;
        case OP_INT_OR:
        case OP_INT_OR_IMM:
            return ALU_OR;
        case OP_INT_XOR:
        case OP_INT_XOR_IMM:
            return ALU_XOR;
        /* uint64 family: identical payload ops, result tag differs (see is_unsigned_op) */
        case OP_UINT_ADD:
        case OP_UINT_ADD_IMM:
            return ALU_ADD;
        case OP_UINT_SUB:
        case OP_UINT_SUB_IMM:
            return ALU_SUB;
        case OP_UINT_MUL:
        case OP_UINT_MUL_IMM:
            return ALU_MUL;
        case OP_UINT_AND:
        case OP_UINT_AND_IMM:
            return ALU_AND;
        case OP_UINT_OR:
        case OP_UINT_OR_IMM:
            return ALU_OR;
        case OP_UINT_XOR:
        case OP_UINT_XOR_IMM:
            return ALU_XOR;
        default:
            return -1;
    }
}

/* True for the uint64-family opcodes reaching the default branch: their immediate
 * operands are zero-extended (I_VAL, not I_SVAL) and their result tag is UINT64. */
static int is_unsigned_op(OpCode op)
{
    switch (op) {
        case OP_UINT_ADD:
        case OP_UINT_ADD_IMM:
        case OP_UINT_SUB:
        case OP_UINT_SUB_IMM:
        case OP_UINT_MUL:
        case OP_UINT_MUL_IMM:
        case OP_UINT_AND:
        case OP_UINT_AND_IMM:
        case OP_UINT_OR:
        case OP_UINT_OR_IMM:
        case OP_UINT_XOR:
        case OP_UINT_XOR_IMM:
        case OP_UINT_EQ_IMM:
        case OP_UINT_NE_IMM:
        case OP_UINT_LT:
        case OP_UINT_LT_IMM:
        case OP_UINT_LE:
        case OP_UINT_LE_IMM:
        case OP_UINT_GT:
        case OP_UINT_GT_IMM:
        case OP_UINT_GE:
        case OP_UINT_GE_IMM:
        case OP_JMP_UINT_LT:
        case OP_JMP_UINT_LT_IMM:
        case OP_JMP_UINT_LE:
        case OP_JMP_UINT_LE_IMM:
        case OP_JMP_UINT_GT:
        case OP_JMP_UINT_GT_IMM:
        case OP_JMP_UINT_GE:
        case OP_JMP_UINT_GE_IMM:
            return 1;
        default:
            return 0;
    }
}

/* True for the *_IMM forms (immediate operand in the low 8 bits). */
static int is_imm_op(OpCode op)
{
    switch (op) {
        case OP_INT_ADD_IMM:
        case OP_INT_SUB_IMM:
        case OP_INT_MUL_IMM:
        case OP_INT_DIV_IMM:
        case OP_INT_MOD_IMM:
        case OP_INT_AND_IMM:
        case OP_INT_OR_IMM:
        case OP_INT_XOR_IMM:
        case OP_INT_SHL_IMM:
        case OP_INT_SHR_IMM:
        case OP_INT_EQ_IMM:
        case OP_INT_NE_IMM:
        case OP_INT_LT_IMM:
        case OP_INT_LE_IMM:
        case OP_INT_GT_IMM:
        case OP_INT_GE_IMM:
        case OP_JMP_INT_EQ_IMM:
        case OP_JMP_INT_NE_IMM:
        case OP_JMP_INT_LT_IMM:
        case OP_JMP_INT_LE_IMM:
        case OP_JMP_INT_GT_IMM:
        case OP_JMP_INT_GE_IMM:
            return 1;
        case OP_UINT_ADD_IMM:
        case OP_UINT_SUB_IMM:
        case OP_UINT_MUL_IMM:
        case OP_UINT_DIV_IMM:
        case OP_UINT_MOD_IMM:
        case OP_UINT_AND_IMM:
        case OP_UINT_OR_IMM:
        case OP_UINT_XOR_IMM:
        case OP_UINT_SHL_IMM:
        case OP_UINT_SHR_IMM:
        case OP_UINT_EQ_IMM:
        case OP_UINT_NE_IMM:
        case OP_UINT_LT_IMM:
        case OP_UINT_LE_IMM:
        case OP_UINT_GT_IMM:
        case OP_UINT_GE_IMM:
        case OP_JMP_UINT_LT_IMM:
        case OP_JMP_UINT_LE_IMM:
        case OP_JMP_UINT_GT_IMM:
        case OP_JMP_UINT_GE_IMM:
            return 1;
        default:
            return 0;
    }
}

/* True for the fused compare-and-branch opcodes (OP_JMP_INT_*). */
static int is_jmp_op(OpCode op)
{
    switch (op) {
        case OP_JMP_INT_EQ:
        case OP_JMP_INT_NE:
        case OP_JMP_INT_LT:
        case OP_JMP_INT_LE:
        case OP_JMP_INT_GT:
        case OP_JMP_INT_GE:
        case OP_JMP_INT_EQ_IMM:
        case OP_JMP_INT_NE_IMM:
        case OP_JMP_INT_LT_IMM:
        case OP_JMP_INT_LE_IMM:
        case OP_JMP_INT_GT_IMM:
        case OP_JMP_INT_GE_IMM:
            return 1;
        case OP_JMP_UINT_LT:
        case OP_JMP_UINT_LE:
        case OP_JMP_UINT_GT:
        case OP_JMP_UINT_GE:
        case OP_JMP_UINT_LT_IMM:
        case OP_JMP_UINT_LE_IMM:
        case OP_JMP_UINT_GT_IMM:
        case OP_JMP_UINT_GE_IMM:
            return 1;
        default:
            return 0;
    }
}

/* ---- per-instruction translation ---- */

/* Returns the number of bytecode words consumed (1, or 2 for OP_CALL+payload).
 * self_only: every call in this function targets itself, so OP_CALL lowers to a
 * direct native self-call instead of the runtime helper. */
static int translate_insn(JitCtx *j, uint32_t insn, int pc, ModuleObject *m, CodeObject *code,
                          int self_only)
{
    EmitBuf *b = &j->buf;
    OpCode op = (OpCode)I_OP(insn);
    int rd, rs, rt, imm, ti, off;

    switch (op) {
        case OP_MOVE:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
            emit_mov_reg_mem(b, RDX, RBX, VAL_DISP(rs));
            emit_mov_mem_reg(b, RBX, TAG_DISP(rd), RAX);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RDX);
            break;

        case OP_LOAD_INT_IMM:
            rd = I_VAL(insn, 16, 8);
            ti = I_VAL(insn, 12, 4);
            imm = sval(I_VAL(insn, 0, 12), 12);
            emit_mov_mem_imm32(b, RBX, TAG_DISP(rd), ti);
            emit_mov_mem_imm32(b, RBX, VAL_DISP(rd), imm);
            break;

        case OP_LOAD_UINT_IMM:
            rd = I_VAL(insn, 16, 8);
            ti = I_VAL(insn, 12, 4);
            imm = (int)I_VAL(insn, 0, 12);
            emit_mov_mem_imm32(b, RBX, TAG_DISP(rd), ti);
            emit_mov_mem_imm32(b, RBX, VAL_DISP(rd), imm);
            break;

        case OP_LOADK: {
            /* Snapshot the constant pool slot at compile time (constants are
             * immutable for the module lifetime); embed tag + payload bits. */
            rd = I_VAL(insn, 16, 8);
            int idx = I_VAL(insn, 0, 16);
            TValue *pool = VECTOR_ITEMS(&m->const_pool, TValue);
            TValue cv = pool[idx];
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
            emit_store_tag(j, rd, (int32_t)cv.tag);
            break;
        }

        case OP_LOAD_TAG: {
            rd = I_VAL(insn, 8, 12);
            int tag = I_VAL(insn, 0, 8);
            TValue cv = kl_jit_tag_value(tag);
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RBX, VAL_DISP(rd), RAX);
            emit_store_tag(j, rd, (int32_t)cv.tag);
            break;
        }

        case OP_INT_NEG:
        case OP_INT_NOT:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
            if (op == OP_INT_NEG)
                emit_neg_reg(b, RAX);
            else
                emit_not_reg(b, RAX);
            emit_store_int64(j, rd);
            break;

        case OP_INT_SHL_IMM:
        case OP_INT_SHR_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = sval(I_VAL(insn, 0, 8), 8);
            emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
            if (op == OP_INT_SHL_IMM)
                emit_shl_reg_imm8(b, RAX, (uint8_t)imm);
            else
                emit_sar_reg_imm8(b, RAX, (uint8_t)imm);
            emit_store_int64(j, rd);
            break;

        case OP_UINT_SHL_IMM:
        case OP_UINT_SHR_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = (int)I_VAL(insn, 0, 8);
            emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
            if (op == OP_UINT_SHL_IMM)
                emit_shl_reg_imm8(b, RAX, (uint8_t)imm);
            else
                emit_shr_reg_imm8(b, RAX, (uint8_t)imm);
            emit_store_tagged(j, rd, TAG_UINT64);
            break;

        case OP_INT_SHL:
        case OP_INT_SHR:
        case OP_UINT_SHL:
        case OP_UINT_SHR: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            int kind = (op == OP_INT_SHR) ? 1 : (op == OP_UINT_SHR) ? 2 : 0;
            int32_t stag =
                (op == OP_UINT_SHL || op == OP_UINT_SHR) ? TAG_UINT64 : TAG_INT64;
            emit_reg_shift(j, rd, rs, rt, kind, stag, code->cs.start_pc + pc);
            break;
        }

        case OP_INT_DIV:
        case OP_INT_MOD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_div_rrr(j, rd, rs, rt, op == OP_INT_MOD, 0);
            break;

        case OP_INT_DIV_IMM:
        case OP_INT_MOD_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = sval(I_VAL(insn, 0, 8), 8);
            emit_div_rrimm(j, rd, rs, imm, op == OP_INT_MOD_IMM, 0);
            break;

        case OP_UINT_DIV:
        case OP_UINT_MOD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_div_rrr(j, rd, rs, rt, op == OP_UINT_MOD, 1);
            break;

        case OP_UINT_DIV_IMM:
        case OP_UINT_MOD_IMM:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            imm = (int)I_VAL(insn, 0, 8);
            emit_div_rrimm(j, rd, rs, imm, op == OP_UINT_MOD_IMM, 1);
            break;

        case OP_INT_CAST: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            int flag = I_VAL(insn, 0, 8);
            emit_cast_call(j, rd, rs, flag & 0x3, (flag >> 2) & 0x3F,
                           (void *)do_int_cast);
            break;
        }

        case OP_FLOAT_CAST:
        case OP_FLOAT_TO_INT:
        case OP_INT_TO_FLOAT: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            int flag = I_VAL(insn, 0, 8);
            void *fn = op == OP_FLOAT_CAST ? (void *)do_float_cast
                     : op == OP_FLOAT_TO_INT ? (void *)do_float_to_int
                                             : (void *)do_int_to_float;
            emit_cast_call(j, rd, rs, flag & 0x3, (flag >> 2) & 0x3F, fn);
            break;
        }

        case OP_FLOAT_ADD:
        case OP_FLOAT_SUB:
        case OP_FLOAT_MUL:
        case OP_FLOAT_DIV: {
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            int sop = op == OP_FLOAT_ADD ? SSE_ADD
                    : op == OP_FLOAT_SUB ? SSE_SUB
                    : op == OP_FLOAT_MUL ? SSE_MUL
                                         : SSE_DIV;
            emit_float_arith(j, sop, rd, rs, rt);
            break;
        }

        case OP_FLOAT_MOD:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_float_mod(j, rd, rs, rt);
            break;

        case OP_FLOAT_NEG:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_float_neg(j, rd, rs);
            break;

        case OP_FLOAT_EQ:
        case OP_FLOAT_NE:
        case OP_FLOAT_LT:
        case OP_FLOAT_LE:
        case OP_FLOAT_GT:
        case OP_FLOAT_GE:
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_float_cmp_bool(j, rd, rs, rt, float_cmp_op(op));
            break;

        case OP_JMP_FLOAT_EQ:
        case OP_JMP_FLOAT_NE:
        case OP_JMP_FLOAT_LT:
        case OP_JMP_FLOAT_LE:
        case OP_JMP_FLOAT_GT:
        case OP_JMP_FLOAT_GE:
            rs = I_VAL(insn, 16, 8);
            rt = I_VAL(insn, 8, 8);
            off = sval(I_VAL(insn, 0, 8), 8);
            emit_float_jmp_cmp(j, rs, rt, float_cmp_op(op), pc + 1 + off);
            break;

        case OP_LAND:
        case OP_LOR:
            /* bool payload is 0/1, so logical and/or == bitwise and/or */
            rd = I_VAL(insn, 16, 8);
            rs = I_VAL(insn, 8, 8);
            rt = I_VAL(insn, 0, 8);
            emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
            emit_alu_reg_mem(b, op == OP_LAND ? ALU_AND : ALU_OR, RAX, RBX,
                             VAL_DISP(rt));
            emit_store_tagged(j, rd, TAG_BOOL);
            break;

        case OP_LNOT:
            rd = I_VAL(insn, 12, 12);
            rs = I_VAL(insn, 0, 12);
            emit_mov_reg_mem(b, RAX, RBX, VAL_DISP(rs));
            emit_alu_reg_imm32(b, ALU_XOR, RAX, 1);
            emit_store_tagged(j, rd, TAG_BOOL);
            break;

        case OP_RET:
            rs = I_VAL(insn, 0, 12);
            emit_load_out(j);
            emit_mov_reg_mem(b, RAX, RBX, TAG_DISP(rs));
            emit_mov_reg_mem(b, RDX, RBX, VAL_DISP(rs));
            emit_mov_mem_reg(b, RCX, 0, RAX);
            emit_mov_mem_reg(b, RCX, 8, RDX);
            emit_epilogue(j);
            break;

        case OP_RET_INT_IMM:
            ti = I_VAL(insn, 16, 8);
            imm = sval(I_VAL(insn, 0, 16), 16);
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, ti);
            emit_mov_mem_imm32(b, RCX, 8, imm);
            emit_epilogue(j);
            break;

        case OP_RET_UINT_IMM:
            ti = I_VAL(insn, 16, 8);
            imm = (int)I_VAL(insn, 0, 16);
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, ti);
            emit_mov_mem_imm32(b, RCX, 8, imm);
            emit_epilogue(j);
            break;

        case OP_RET_TAG: {
            int tag = I_VAL(insn, 0, 8);
            TValue cv = kl_jit_tag_value(tag);
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, (int32_t)cv.tag);
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RCX, 8, RAX);
            emit_epilogue(j);
            break;
        }

        case OP_RET_CONST: {
            int idx = I_VAL(insn, 0, 16);
            TValue *pool = VECTOR_ITEMS(&m->const_pool, TValue);
            TValue cv = pool[idx];
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, (int32_t)cv.tag);
            emit_mov_reg_imm64(b, RAX, (uint64_t)cv.ival);
            emit_mov_mem_reg(b, RCX, 8, RAX);
            emit_epilogue(j);
            break;
        }

        case OP_RET_VOID:
            emit_load_out(j);
            emit_mov_mem_imm32(b, RCX, 0, TAG_NIL);
            emit_mov_mem_imm32(b, RCX, 8, 0);
            emit_epilogue(j);
            break;

        case OP_JMP:
            off = sval(I_VAL(insn, 0, 16), 16);
            emit_jump(j, -1, pc + 1 + off);
            break;

        case OP_JMP_TRUE:
        case OP_JMP_FALSE:
            rd = I_VAL(insn, 16, 8);
            off = sval(I_VAL(insn, 0, 16), 16);
            emit_alu_mem_imm32(b, ALU_CMP, RBX, VAL_DISP(rd), 0);
            emit_jump(j, op == OP_JMP_TRUE ? CC_NE : CC_E, pc + 1 + off);
            break;

        case OP_CALL: {
            /* Two words: | op:8 | flag:4 | rd:12 | nargs:8 | + payload(entry index).
             * v1 lowers flag=0 ordinary calls (local CodeObject or imported
             * CFuncObject) through the runtime helper; flag=1 (interface) and
             * flag=2 (register callee) resolve dynamically and bail for now. */
            int flg = I_VAL(insn, 20, 4);
            rd = I_VAL(insn, 8, 12);
            int nargs = I_VAL(insn, 0, 8);
            if (flg != 0) {
                j->failed = 1;
                j->fail_pc = pc;
                j->fail_insn = insn;
                return 2;
            }
            if (self_only) {
                emit_self_call(j, rd, code->cs.nlocals);
                return 2;
            }
            uint32_t *insns = m->codes + code->cs.start_pc;
            int32_t entry_index = (int32_t)insns[pc + 1];
            Object **entry_table = VECTOR_ITEMS(&m->funcs, Object *);
            Object *fn = entry_table[entry_index];
            emit_helper_call(j, rd, nargs, code->cs.nlocals, fn,
                             code->cs.start_pc + pc);
            return 2; /* consumed the payload word as well */
        }

        default: {
            int alu = arith_alu(op);
            int cc = cmp_cc(op);
            int uns = is_unsigned_op(op);
            int32_t rtag = uns ? TAG_UINT64 : TAG_INT64;
            if (alu >= 0) {
                /* FORMAT_RRR (rd@16, rs@8, rt@0) or RRImm (rd@16, rs@8, imm@0).
                 * uint immediates are zero-extended (I_VAL), int sign-extended. */
                rd = I_VAL(insn, 16, 8);
                rs = I_VAL(insn, 8, 8);
                if (is_imm_op(op)) {
                    imm = uns ? (int)I_VAL(insn, 0, 8) : sval(I_VAL(insn, 0, 8), 8);
                    emit_alu_rrimm(j, alu, rd, rs, imm, rtag);
                } else {
                    rt = I_VAL(insn, 0, 8);
                    emit_alu_rrr(j, alu, rd, rs, rt, rtag);
                }
            } else if (cc >= 0 && is_jmp_op(op)) {
                /* FORMAT_RROff (rs@16, rt@8, off@0) or RImmOff (rs@16, imm@8, off@0). */
                rs = I_VAL(insn, 16, 8);
                off = sval(I_VAL(insn, 0, 8), 8);
                if (is_imm_op(op)) {
                    imm = uns ? (int)I_VAL(insn, 8, 8) : sval(I_VAL(insn, 8, 8), 8);
                    emit_jmp_cmp_rrimm(j, cc, rs, imm, pc + 1 + off);
                } else {
                    rt = I_VAL(insn, 8, 8);
                    emit_jmp_cmp_rrr(j, cc, rs, rt, pc + 1 + off);
                }
            } else if (cc >= 0) {
                /* Comparison producing a BOOL: RRR (rd@16, rs@8, rt@0) or RRImm. */
                rd = I_VAL(insn, 16, 8);
                rs = I_VAL(insn, 8, 8);
                if (is_imm_op(op)) {
                    imm = uns ? (int)I_VAL(insn, 0, 8) : sval(I_VAL(insn, 0, 8), 8);
                    emit_int_cmp_rrimm(j, cc, rd, rs, imm);
                } else {
                    rt = I_VAL(insn, 0, 8);
                    emit_int_cmp_rrr(j, cc, rd, rs, rt);
                }
            } else {
                /* Unsupported opcode: bail out, keep the interpreter. */
                j->failed = 1;
                j->fail_pc = pc;
                j->fail_insn = insn;
            }
            break;
        }
    }
    return 1;
}

/* True when every OP_CALL in the function targets the function itself (flag=0).
 * Pure self-recursion never enters kl_call_code, so ks->stack_top is never read
 * during the recursion and direct native self-calls are safe. Any non-self call
 * (or an unsupported flag) disqualifies the function: it then uses the runtime
 * helper for all calls, which keeps stack_top consistent. */
static int is_self_only(ModuleObject *m, CodeObject *code, const uint32_t *insns, int n)
{
    Object **entry_table = VECTOR_ITEMS(&m->funcs, Object *);
    for (int pc = 0; pc < n; pc++) {
        uint32_t insn = insns[pc];
        if (I_OP(insn) != OP_CALL) continue;
        if (I_VAL(insn, 20, 4) != 0) return 0;
        int32_t entry_index = (int32_t)insns[pc + 1];
        if (entry_table[entry_index] != (Object *)code) return 0;
        pc++; /* skip the payload word */
    }
    return 1;
}

/* ---- compilation entry point ---- */

JitEntry *kl_jit_compile(CodeObject *code)
{
    if (!kl_jit_enabled()) return NULL;

    const char *name = code->cs.name ? code->cs.name : "?";

    /* A free function's owner is its ModuleObject; a method's owner is a
     * TypeObject. v1 only handles module-owned free functions, so anything else
     * (methods like BufWriter$write, ...) is skipped silently. */
    Object *owner = code->owner;
    if (!owner || !IS_MODULE(owner)) return NULL;
    ModuleObject *m = (ModuleObject *)owner;

    /* Only the module's OWN functions: funcs[0..num_funcs). resolve_import
     * appends imported functions at index >= num_funcs, so they are filtered
     * here silently and fast-passed via jit_tried, neither compiling nor
     * cluttering the trace. */
    if (!kl_mo_is_local_func((Object *)m, (Object *)code)) return NULL;

    if (!m->codes) {
        if (jit_trace()) jit_print_bail(name, -1, 0, "module has no code", stderr);
        return NULL;
    }

    int n = code->cs.num_insns;
    int start = code->cs.start_pc;
    if (n <= 0) {
        if (jit_trace()) jit_print_bail(name, -1, 0, "empty code", stderr);
        return NULL;
    }
    uint32_t *insns = m->codes + start;

    JitCtx j;
    memset(&j, 0, sizeof(j));
    j.num_insns = n;
    j.pc_map = calloc(n, sizeof(size_t));
    j.fixups = calloc(n, sizeof(Fixup));
    emit_init(&j.buf);

    emit_prologue(&j);

    int self_only = is_self_only(m, code, insns, n);
    for (int pc = 0; pc < n && !j.failed; pc++) {
        j.pc_map[pc] = emit_pos(&j.buf);
        pc += translate_insn(&j, insns[pc], pc, m, code, self_only) - 1;
    }

    JitEntry *entry = NULL;
    if (j.failed) {
        if (jit_trace()) jit_print_bail(name, j.fail_pc, j.fail_insn, "unsupported opcode", stderr);
        goto done;
    }

    /* Error epilogue: entered from any helper call that returned TAG_ERROR.
     * Write error_value to *out and return, mirroring the interpreter's error
     * path (the frame's traceback entry was already recorded by the helper). */
    size_t err_off = emit_pos(&j.buf);
    emit_load_out(&j);
    emit_mov_mem_imm32(&j.buf, RCX, 0, TAG_ERROR);
    emit_mov_mem_imm32(&j.buf, RCX, 8, -1);
    emit_epilogue(&j);

    /* Patch jump displacements now that every pc has a native offset. */
    for (int i = 0; i < j.nfixups; i++) {
        int t = j.fixups[i].target_pc;
        size_t target_off;
        if (t == JIT_ERR_TARGET) {
            target_off = err_off;
        } else if (t >= 0 && t < n) {
            target_off = j.pc_map[t];
        } else {
            j.failed = 1;
            if (jit_trace()) jit_print_bail(name, -1, 0, "jump target out of range", stderr);
            goto done;
        }
        int32_t rel = (int32_t)(target_off - (j.fixups[i].patch_off + 4));
        emit_patch_i32(&j.buf, j.fixups[i].patch_off, rel);
    }

    /* W^X: write into a RW mapping, then flip to RX. */
    size_t sz = j.buf.len;
    void *mem = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        if (jit_trace()) jit_print_bail(name, -1, 0, "mmap failed", stderr);
        goto done;
    }
    memcpy(mem, j.buf.data, sz);
    if (mprotect(mem, sz, PROT_READ | PROT_EXEC) != 0) {
        munmap(mem, sz);
        if (jit_trace()) jit_print_bail(name, -1, 0, "mprotect failed", stderr);
        goto done;
    }

    entry = calloc(1, sizeof(JitEntry));
    entry->fn = (void (*)(TValue *, TValue *))mem;
    entry->mem = mem;
    entry->mem_size = sz;

    if (jit_trace()) jit_print_code(name, insns, n, j.pc_map, j.buf.data, j.buf.len, err_off, stderr);

done:
    free(j.pc_map);
    free(j.fixups);
    emit_free(&j.buf);
    return entry;
}

void kl_jit_free(JitEntry *e)
{
    if (!e) return;
    if (e->mem) munmap(e->mem, e->mem_size);
    free(e);
}

#ifdef __cplusplus
}
#endif
