/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CPU_X86_64_OPS_H_
#define _KOALA_CPU_X86_64_OPS_H_

/*
 * x86-64 instruction definitions for the JIT disassembler (X-Macro source of
 * truth), modelled on include/common/opcode_list.h. disasm.c expands these
 * lists into lookup tables; no opcode value or mnemonic is hardcoded there.
 *
 * Only the subset emitted by emit.c is described. Intel syntax.
 *
 * Main instruction table entry:
 *     X(NAME, opcode, mask, ENC, "mnemonic")
 *
 * Fields:
 *     opcode   — value to match against the (second) opcode byte
 *     mask     — bits compared: (byte & mask) == opcode selects the entry.
 *                0xFF = exact; 0xF8 = low-3-bits carry a register (push/pop/
 *                mov-imm64); 0xF0 = low nibble carries a condition code
 *                (jcc/setcc); 0xC7 = bits[5:3] carry the ALU op (03+op*8).
 *     ENC      — X86Enc operand-encoding class (see enum below); drives how
 *                the remaining bytes (ModR/M, disp, imm, rel32) are parsed.
 *     mnemonic — display string. "" means the mnemonic is derived from a
 *                group/cc table at decode time (ALU, F7, shift, jcc, setcc).
 *
 * Two-byte opcodes (0F xx) live in X86_OPS_0F_LIST, matched on the second byte.
 */

/* Operand-encoding classes (the x86 analogue of OpFormat). */
typedef enum {
    X86_ENC_NONE,         /* no operands: ret, cqo                       */
    X86_ENC_RM_REG,       /* ModR/M: dst = r/m, src = reg     (mov 89)   */
    X86_ENC_REG_RM,       /* ModR/M: dst = reg, src = r/m     (mov 8B)   */
    X86_ENC_ALU_REG_RM,   /* reg op r/m; mn = ALU[(opcode>>3)&7]         */
    X86_ENC_RM_IMM32,     /* ModR/M: dst = r/m, imm32         (mov C7)   */
    X86_ENC_ALU_RM_IMM32, /* r/m op imm32; mn = ALU[reg]      (81)       */
    X86_ENC_REG_IMM64,    /* opcode-reg, imm64                (B8+r)     */
    X86_ENC_GRP_RM,       /* ModR/M: r/m; mn = F7[reg]        (F7)       */
    X86_ENC_GRP_RM_IMM8,  /* ModR/M: r/m, imm8; mn = SHIFT[reg] (C1)     */
    X86_ENC_GRP_RM_CL,    /* ModR/M: r/m, cl; mn = SHIFT[reg]   (D3)     */
    X86_ENC_SETCC,        /* ModR/M: r/m8; mn = "set" + CC[opcode&15]    */
    X86_ENC_MOVZX,        /* ModR/M: dst = reg, src = r/m8    (0F B6)    */
    X86_ENC_JMP_REL32,    /* rel32 target                     (E9)       */
    X86_ENC_JCC_REL32,    /* rel32; mn = "j" + CC[opcode&15]  (0F 8x)    */
    X86_ENC_CALL_REL32,   /* rel32 target                     (E8)       */
    X86_ENC_CALL_RM,      /* ModR/M: r/m; /2 = call           (FF /2)    */
    X86_ENC_PUSH,         /* opcode-reg                       (50+r)     */
    X86_ENC_POP,          /* opcode-reg                       (58+r)     */
    X86_ENC_SSE_REG_RM,   /* F2 0F: dst = xmm, src = r/m      (movsd 10) */
    X86_ENC_SSE_RM_REG,   /* F2 0F: dst = r/m, src = xmm      (movsd 11) */
    X86_ENC_MOVQ_XMM_GPR, /* 66 REX.W 0F 6E: dst = xmm, src = r64        */
    X86_ENC_MOVQ_GPR_XMM, /* 66 REX.W 0F 7E: dst = r64 (r/m), src = xmm  */
} X86Enc;

/* ------------------------------------------------------------------ +
 |  Registers: X(index, r64, r8_with_rex, r8_legacy)                  |
 |  r8_legacy applies only to indices 0-7 when no REX prefix is present|
 |  (spl/bpl/sil/dil require REX; otherwise ah/ch/dh/bh).              |
 + ------------------------------------------------------------------ */
#define X86_REG_LIST \
    X(0,  "rax", "al",   "al")   \
    X(1,  "rcx", "cl",   "cl")   \
    X(2,  "rdx", "dl",   "dl")   \
    X(3,  "rbx", "bl",   "bl")   \
    X(4,  "rsp", "spl",  "ah")   \
    X(5,  "rbp", "bpl",  "ch")   \
    X(6,  "rsi", "sil",  "dh")   \
    X(7,  "rdi", "dil",  "bh")   \
    X(8,  "r8",  "r8b",  "r8b")  \
    X(9,  "r9",  "r9b",  "r9b")  \
    X(10, "r10", "r10b", "r10b") \
    X(11, "r11", "r11b", "r11b") \
    X(12, "r12", "r12b", "r12b") \
    X(13, "r13", "r13b", "r13b") \
    X(14, "r14", "r14b", "r14b") \
    X(15, "r15", "r15b", "r15b")

/* XMM registers: X(index, name). */
#define X86_XMM_LIST \
    X(0, "xmm0") X(1, "xmm1") X(2, "xmm2") X(3, "xmm3") \
    X(4, "xmm4") X(5, "xmm5") X(6, "xmm6") X(7, "xmm7") \
    X(8, "xmm8") X(9, "xmm9") X(10, "xmm10") X(11, "xmm11") \
    X(12, "xmm12") X(13, "xmm13") X(14, "xmm14") X(15, "xmm15")

/* ------------------------------------------------------------------ +
 |  Condition codes: X(cc, suffix) — low nibble of Jcc/SETcc           |
 + ------------------------------------------------------------------ */
#define X86_CC_LIST \
    X(0x0, "o")  X(0x1, "no") X(0x2, "b")  X(0x3, "ae") \
    X(0x4, "e")  X(0x5, "ne") X(0x6, "be") X(0x7, "a")  \
    X(0x8, "s")  X(0x9, "ns") X(0xA, "p")  X(0xB, "np") \
    X(0xC, "l")  X(0xD, "ge") X(0xE, "le") X(0xF, "g")

/* ------------------------------------------------------------------ +
 |  ALU group: X(op, mnemonic) — opcode 03+op*8 and the /reg field of 81|
 + ------------------------------------------------------------------ */
#define X86_ALU_LIST \
    X(0, "add") X(1, "or")  X(2, "adc") X(3, "sbb") \
    X(4, "and") X(5, "sub") X(6, "xor") X(7, "cmp")

/* ------------------------------------------------------------------ +
 |  F7 unary group: X(reg_field, mnemonic) — sparse, indexed by /reg    |
 + ------------------------------------------------------------------ */
#define X86_GRP_F7_LIST \
    X(2, "not") X(3, "neg") X(6, "div") X(7, "idiv")

/* ------------------------------------------------------------------ +
 |  Shift group: X(reg_field, mnemonic) — shared by C1 (imm8) / D3 (cl)|
 + ------------------------------------------------------------------ */
#define X86_GRP_SHIFT_LIST \
    X(4, "shl") X(5, "shr") X(7, "sar")

/* ------------------------------------------------------------------ +
 |  Primary opcode map (single-byte opcodes)                          |
 + ------------------------------------------------------------------ */

/* mov r/m, reg — store register to register/memory. */
#define X86_OPS_LIST \
    X(X86_MOV_RM_REG,    0x89, 0xFF, X86_ENC_RM_REG,       "mov")  \
    X(X86_MOV_REG_RM,    0x8B, 0xFF, X86_ENC_REG_RM,       "mov")  \
    X(X86_MOV_RM_IMM32,  0xC7, 0xFF, X86_ENC_RM_IMM32,     "mov")  \
    X(X86_MOV_REG_IMM64, 0xB8, 0xF8, X86_ENC_REG_IMM64,    "mov")  \
    X(X86_ALU_REG_RM,    0x03, 0xC7, X86_ENC_ALU_REG_RM,   "")     \
    X(X86_ALU_RM_IMM32,  0x81, 0xFF, X86_ENC_ALU_RM_IMM32, "")     \
    X(X86_GRP_F7,        0xF7, 0xFF, X86_ENC_GRP_RM,       "")     \
    X(X86_SHIFT_IMM8,    0xC1, 0xFF, X86_ENC_GRP_RM_IMM8,  "")     \
    X(X86_SHIFT_CL,      0xD3, 0xFF, X86_ENC_GRP_RM_CL,    "")     \
    X(X86_CQO,           0x99, 0xFF, X86_ENC_NONE,         "cqo")  \
    X(X86_RET,           0xC3, 0xFF, X86_ENC_NONE,         "ret")  \
    X(X86_JMP,           0xE9, 0xFF, X86_ENC_JMP_REL32,    "jmp")  \
    X(X86_CALL_REL32,    0xE8, 0xFF, X86_ENC_CALL_REL32,   "call") \
    X(X86_CALL_RM,       0xFF, 0xFF, X86_ENC_CALL_RM,      "call") \
    X(X86_PUSH,          0x50, 0xF8, X86_ENC_PUSH,         "push") \
    X(X86_POP,           0x58, 0xF8, X86_ENC_POP,          "pop")

/* ------------------------------------------------------------------ +
 |  Two-byte opcode map (0F prefix); matched on the second byte        |
 + ------------------------------------------------------------------ */
#define X86_OPS_0F_LIST \
    X(X86_IMUL,  0xAF, 0xFF, X86_ENC_REG_RM,    "imul")  \
    X(X86_MOVZX, 0xB6, 0xFF, X86_ENC_MOVZX,     "movzx") \
    X(X86_JCC,   0x80, 0xF0, X86_ENC_JCC_REL32, "j")     \
    X(X86_SETCC, 0x90, 0xF0, X86_ENC_SETCC,     "set")   \
    X(X86_MOVAPS, 0x28, 0xFF, X86_ENC_SSE_REG_RM, "movaps") \
    X(X86_UD2,   0x0B, 0xFF, X86_ENC_NONE,      "ud2")

/* ------------------------------------------------------------------ +
 |  SSE2 scalar double (F2 prefix + 0F xx); matched on the 0F byte      |
 + ------------------------------------------------------------------ */
#define X86_OPS_SSE_LIST \
    X(X86_MOVSD_LOAD,  0x10, 0xFF, X86_ENC_SSE_REG_RM, "movsd") \
    X(X86_MOVSD_STORE, 0x11, 0xFF, X86_ENC_SSE_RM_REG, "movsd") \
    X(X86_ADDSD,       0x58, 0xFF, X86_ENC_SSE_REG_RM, "addsd") \
    X(X86_MULSD,       0x59, 0xFF, X86_ENC_SSE_REG_RM, "mulsd") \
    X(X86_SUBSD,       0x5C, 0xFF, X86_ENC_SSE_REG_RM, "subsd") \
    X(X86_DIVSD,       0x5E, 0xFF, X86_ENC_SSE_REG_RM, "divsd")

/* ------------------------------------------------------------------ +
 |  SSE2 packed/compare (66 prefix + 0F xx); matched on the 0F byte    |
 + ------------------------------------------------------------------ */
#define X86_OPS_SSE66_LIST \
    X(X86_UCOMISD, 0x2E, 0xFF, X86_ENC_SSE_REG_RM,   "ucomisd") \
    X(X86_MOVQ_X_G, 0x6E, 0xFF, X86_ENC_MOVQ_XMM_GPR, "movq")    \
    X(X86_MOVQ_G_X, 0x7E, 0xFF, X86_ENC_MOVQ_GPR_XMM, "movq")

#endif /* _KOALA_CPU_X86_64_OPS_H_ */