/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Minimal x86-64 disassembler for the instruction subset emitted by emit.c.
 * Table-driven: every opcode, mnemonic, register/condition/group name comes
 * from the X-Macro definitions in cpu_x86_64_ops.h (modelled on the VM's
 * opcode_list.h). This file only holds the generic decode/format algorithm —
 * REX + ModR/M + displacement parsing and a switch on the operand-encoding
 * class. Intel syntax, self-contained, no dependency on libparser.
 */

#include "disasm.h"
#include "cpu_x86_64_ops.h"

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- name tables expanded from the X-Macro lists ---- */

static const char *reg64_name[] = {
#define X(i, r64, r8r, r8l) [i] = r64,
    X86_REG_LIST
#undef X
};

static const char *reg8_rex[] = {
#define X(i, r64, r8r, r8l) [i] = r8r,
    X86_REG_LIST
#undef X
};

static const char *reg8_legacy[] = {
#define X(i, r64, r8r, r8l) [i] = r8l,
    X86_REG_LIST
#undef X
};

static const char *xmm_name[] = {
#define X(i, nm) [i] = nm,
    X86_XMM_LIST
#undef X
};

static const char *cc_suffix[] = {
#define X(cc, s) [cc] = s,
    X86_CC_LIST
#undef X
};

static const char *alu_mn[] = {
#define X(op, mn) [op] = mn,
    X86_ALU_LIST
#undef X
};

static const char *grp_f7[] = {
#define X(reg, mn) [reg] = mn,
    X86_GRP_F7_LIST
#undef X
};

static const char *grp_shift[] = {
#define X(reg, mn) [reg] = mn,
    X86_GRP_SHIFT_LIST
#undef X
};

/* ---- instruction tables ---- */

typedef struct {
    uint8_t op;
    uint8_t mask;
    X86Enc enc;
    const char *mn;
} X86OpDef;

static const X86OpDef ops[] = {
#define X(name, op, mask, enc, mn) {op, mask, enc, mn},
    X86_OPS_LIST
#undef X
};

static const X86OpDef ops0f[] = {
#define X(name, op, mask, enc, mn) {op, mask, enc, mn},
    X86_OPS_0F_LIST
#undef X
};

static const X86OpDef ops_sse[] = {
#define X(name, op, mask, enc, mn) {op, mask, enc, mn},
    X86_OPS_SSE_LIST
#undef X
};

static const X86OpDef *find_op(const X86OpDef *tbl, size_t n, uint8_t byte)
{
    for (size_t i = 0; i < n; i++)
        if ((uint8_t)(byte & tbl[i].mask) == tbl[i].op) return &tbl[i];
    return NULL;
}

/* ---- byte cursor ---- */

typedef struct {
    const uint8_t *p;
    size_t avail;
    size_t pos;
    size_t off; /* absolute offset of the instruction start */
    int bad;
    int rex; /* REX byte, or 0 when absent */
    int rex_w, rex_r, rex_x, rex_b;
    int f2; /* F2 mandatory prefix (SSE2 scalar double) */
} Dis;

static int rd_u8(Dis *d)
{
    if (d->pos >= d->avail) {
        d->bad = 1;
        return 0;
    }
    return d->p[d->pos++];
}

static uint32_t rd_u32(Dis *d)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)rd_u8(d) << (8 * i);
    return v;
}

static uint64_t rd_u64(Dis *d)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)rd_u8(d) << (8 * i);
    return v;
}

static const char *reg64(int r) { return reg64_name[r & 15]; }

static const char *reg8(int r, int has_rex)
{
    if (has_rex) return reg8_rex[r & 15];
    return reg8_legacy[r & 7];
}

/* Operand-size directive for a memory destination (REX.W selects 64-bit). */
static const char *sz_prefix(Dis *d) { return d->rex_w ? "qword" : "dword"; }

/* [base + disp] memory operand. The emitter only uses RBX/RBP bases with a
 * disp8 (mod=01) or disp32 (mod=10); no SIB and no mod=00 forms occur. The
 * displacement is always shown (even +0x0) to mirror the encoded bytes and
 * match objdump. */
static void mem_operand(Dis *d, int mod, int rm, char *out, size_t outsz)
{
    int base = (rm & 7) + (d->rex_b ? 8 : 0);
    int32_t disp = 0;
    int has_disp = 0;
    if (mod == 1) {
        disp = (int8_t)rd_u8(d);
        has_disp = 1;
    } else if (mod == 2) {
        disp = (int32_t)rd_u32(d);
        has_disp = 1;
    }

    if (!has_disp)
        snprintf(out, outsz, "[%s]", reg64(base));
    else if (disp >= 0)
        snprintf(out, outsz, "[%s+0x%x]", reg64(base), (unsigned)disp);
    else
        snprintf(out, outsz, "[%s-0x%x]", reg64(base), (unsigned)(-disp));
}

/* The r/m operand: a register when mod=3, otherwise memory. */
static void rm_operand(Dis *d, int mm, char *out, size_t outsz)
{
    int mod = mm >> 6, rm = mm & 7;
    if (mod == 3)
        snprintf(out, outsz, "%s", reg64(rm + (d->rex_b ? 8 : 0)));
    else
        mem_operand(d, mod, rm, out, outsz);
}

/* Format an imm32 operand. With REX.W the immediate is sign-extended to 64
 * bits (so -7 prints as 0xfffffffffffffff9, matching objdump); otherwise it is
 * a 32-bit value. */
static void fmt_imm32(Dis *d, uint32_t imm, char *out, size_t outsz)
{
    if (d->rex_w)
        snprintf(out, outsz, "0x%lx", (unsigned long)(int64_t)(int32_t)imm);
    else
        snprintf(out, outsz, "0x%x", imm);
}

size_t jit_disasm_one(const uint8_t *p, size_t avail, size_t off, char *out,
                      size_t outsz, size_t *jtarget)
{
    Dis d;
    memset(&d, 0, sizeof(d));
    d.p = p;
    d.avail = avail;
    d.off = off;
    out[0] = '\0';
    if (jtarget) *jtarget = SIZE_MAX;

    int b = rd_u8(&d);
    if (b == 0xF2) { /* SSE2 scalar-double mandatory prefix */
        d.f2 = 1;
        b = rd_u8(&d);
    }
    if (b >= 0x40 && b <= 0x4F) { /* REX prefix */
        d.rex = b;
        d.rex_w = (b >> 3) & 1;
        d.rex_r = (b >> 2) & 1;
        d.rex_x = (b >> 1) & 1;
        d.rex_b = b & 1;
        b = rd_u8(&d);
    }

    /* Match the opcode against the tables. `opcode` is the byte whose low bits
     * may carry a register (push/pop/mov-imm64) or condition code (jcc/setcc),
     * or whose bits[5:3] carry the ALU op. */
    int opcode = b;
    const X86OpDef *def;
    if (b == 0x0F) {
        opcode = rd_u8(&d);
        if (d.f2)
            def = find_op(ops_sse, sizeof(ops_sse) / sizeof(ops_sse[0]), (uint8_t)opcode);
        else
            def = find_op(ops0f, sizeof(ops0f) / sizeof(ops0f[0]), (uint8_t)opcode);
    } else {
        def = find_op(ops, sizeof(ops) / sizeof(ops[0]), (uint8_t)b);
    }
    if (!def) {
        snprintf(out, outsz, "(bad %02x)", b);
        return d.pos ? d.pos : 1;
    }

    int mm, mod, reg, rm;
    char rmstr[64], immstr[32];
    const char *mn = def->mn;

    switch (def->enc) {
        case X86_ENC_NONE:
            snprintf(out, outsz, "%s", mn);
            break;

        case X86_ENC_RM_REG:
            mm = rd_u8(&d);
            reg = ((mm >> 3) & 7) + (d.rex_r ? 8 : 0);
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            snprintf(out, outsz, "%s %s, %s", mn, rmstr, reg64(reg));
            break;

        case X86_ENC_REG_RM:
            mm = rd_u8(&d);
            reg = ((mm >> 3) & 7) + (d.rex_r ? 8 : 0);
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            snprintf(out, outsz, "%s %s, %s", mn, reg64(reg), rmstr);
            break;

        case X86_ENC_ALU_REG_RM:
            mm = rd_u8(&d);
            reg = ((mm >> 3) & 7) + (d.rex_r ? 8 : 0);
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            mn = alu_mn[(opcode >> 3) & 7];
            snprintf(out, outsz, "%s %s, %s", mn, reg64(reg), rmstr);
            break;

        case X86_ENC_RM_IMM32:
            mm = rd_u8(&d);
            mod = mm >> 6;
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            fmt_imm32(&d, rd_u32(&d), immstr, sizeof(immstr));
            if (mod == 3)
                snprintf(out, outsz, "%s %s, %s", mn, rmstr, immstr);
            else
                snprintf(out, outsz, "%s %s %s, %s", mn, sz_prefix(&d), rmstr,
                         immstr);
            break;

        case X86_ENC_ALU_RM_IMM32:
            mm = rd_u8(&d);
            mod = mm >> 6;
            reg = (mm >> 3) & 7;
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            fmt_imm32(&d, rd_u32(&d), immstr, sizeof(immstr));
            mn = alu_mn[reg];
            if (mod == 3)
                snprintf(out, outsz, "%s %s, %s", mn, rmstr, immstr);
            else
                snprintf(out, outsz, "%s %s %s, %s", mn, sz_prefix(&d), rmstr,
                         immstr);
            break;

        case X86_ENC_REG_IMM64: {
            reg = (opcode & 7) + (d.rex_b ? 8 : 0);
            uint64_t imm64 = rd_u64(&d);
            snprintf(out, outsz, "%s %s, 0x%lx", mn, reg64(reg),
                     (unsigned long)imm64);
            break;
        }

        case X86_ENC_GRP_RM:
            mm = rd_u8(&d);
            mod = mm >> 6;
            reg = (mm >> 3) & 7;
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            mn = grp_f7[reg];
            if (!mn) {
                snprintf(out, outsz, "(bad f7 /%d)", reg);
                d.bad = 1;
                break;
            }
            if (mod == 3)
                snprintf(out, outsz, "%s %s", mn, rmstr);
            else
                snprintf(out, outsz, "%s %s %s", mn, sz_prefix(&d), rmstr);
            break;

        case X86_ENC_GRP_RM_IMM8: {
            mm = rd_u8(&d);
            reg = (mm >> 3) & 7;
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            uint32_t sh = (uint8_t)rd_u8(&d);
            mn = grp_shift[reg];
            if (!mn) {
                snprintf(out, outsz, "(bad c1 /%d)", reg);
                d.bad = 1;
                break;
            }
            snprintf(out, outsz, "%s %s, 0x%x", mn, rmstr, sh);
            break;
        }

        case X86_ENC_GRP_RM_CL:
            mm = rd_u8(&d);
            reg = (mm >> 3) & 7;
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            mn = grp_shift[reg];
            if (!mn) {
                snprintf(out, outsz, "(bad d3 /%d)", reg);
                d.bad = 1;
                break;
            }
            snprintf(out, outsz, "%s %s, cl", mn, rmstr);
            break;

        case X86_ENC_SETCC:
            mm = rd_u8(&d);
            rm = (mm & 7) + (d.rex_b ? 8 : 0);
            snprintf(out, outsz, "%s%s %s", mn, cc_suffix[opcode & 15],
                     reg8(rm, d.rex != 0));
            break;

        case X86_ENC_MOVZX:
            mm = rd_u8(&d);
            mod = mm >> 6;
            reg = ((mm >> 3) & 7) + (d.rex_r ? 8 : 0);
            rm = (mm & 7) + (d.rex_b ? 8 : 0);
            if (mod == 3)
                snprintf(out, outsz, "%s %s, %s", mn, reg64(reg),
                         reg8(rm, d.rex != 0));
            else
                snprintf(out, outsz, "%s %s, byte [%s]", mn, reg64(reg),
                         reg64(rm));
            break;

        case X86_ENC_JMP_REL32: {
            int32_t rel = (int32_t)rd_u32(&d);
            size_t target = d.off + d.pos + (int64_t)rel;
            snprintf(out, outsz, "%s 0x%zx", mn, target);
            if (jtarget) *jtarget = target;
            break;
        }

        case X86_ENC_JCC_REL32: {
            int32_t rel = (int32_t)rd_u32(&d);
            size_t target = d.off + d.pos + (int64_t)rel;
            snprintf(out, outsz, "%s%s 0x%zx", mn, cc_suffix[opcode & 15],
                     target);
            if (jtarget) *jtarget = target;
            break;
        }

        case X86_ENC_CALL_REL32: {
            int32_t rel = (int32_t)rd_u32(&d);
            size_t target = d.off + d.pos + (int64_t)rel;
            snprintf(out, outsz, "%s 0x%zx", mn, target);
            if (jtarget) *jtarget = target;
            break;
        }

        case X86_ENC_CALL_RM:
            mm = rd_u8(&d);
            reg = (mm >> 3) & 7;
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            if (reg != 2) {
                snprintf(out, outsz, "(bad ff /%d)", reg);
                d.bad = 1;
                break;
            }
            snprintf(out, outsz, "%s %s", mn, rmstr);
            break;

        case X86_ENC_PUSH:
            reg = (opcode & 7) + (d.rex_b ? 8 : 0);
            snprintf(out, outsz, "%s %s", mn, reg64(reg));
            break;

        case X86_ENC_POP:
            reg = (opcode & 7) + (d.rex_b ? 8 : 0);
            snprintf(out, outsz, "%s %s", mn, reg64(reg));
            break;

        case X86_ENC_SSE_REG_RM:
            mm = rd_u8(&d);
            reg = ((mm >> 3) & 7) + (d.rex_r ? 8 : 0);
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            snprintf(out, outsz, "%s %s, %s", mn, xmm_name[reg & 15], rmstr);
            break;

        case X86_ENC_SSE_RM_REG:
            mm = rd_u8(&d);
            reg = ((mm >> 3) & 7) + (d.rex_r ? 8 : 0);
            rm_operand(&d, mm, rmstr, sizeof(rmstr));
            snprintf(out, outsz, "%s %s, %s", mn, rmstr, xmm_name[reg & 15]);
            break;
    }

    if (d.bad && out[0] == '\0') snprintf(out, outsz, "(bad)");
    return d.pos ? d.pos : 1;
}

#ifdef __cplusplus
}
#endif