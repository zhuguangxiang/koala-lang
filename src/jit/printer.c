/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * JIT trace printer. Dumps a compiled function in the same textual style as the
 * compiler's klr IR printer (see src/parser/klr/printer.c): a `func @name {`
 * header, `%bbN:` block labels, 8-space indented instructions and `;;` notes.
 *
 * The "IR" here is the VM bytecode being lowered. Each bytecode op prints one
 * header line (decoded operands: registers rN, immediates #N, jumps label %bbN)
 * annotated with its native offset and total byte count, followed by one line
 * per emitted native instruction showing its hex bytes and x86-64 disassembly
 * (see disasm.c), aligned under the op. Branch targets are mapped back to %bbN.
 *
 * Opcode name/format tables are generated locally from opcode_list.h via the
 * X-macro, exactly like src/parser/klr/opcode.c, so the runtime does NOT need to
 * link the compiler's parser library (which owns __op_names/__op_formats).
 */

#include "printer.h"
#include "disasm.h"
#include "opcode.h"

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Left column: 8 spaces + "0000: " (6) + op text (OP_WIDTH) + 1 space, so the
 * ";;" note column starts at NOTE_COL. Continuation lines (one per native
 * instruction) pad to the same column to stay aligned under their bytecode op. */
#define OP_WIDTH 44
#define NOTE_COL (14 + OP_WIDTH + 1)
/* Widest emitted instruction is 11 bytes -> 32 hex chars; pad to keep the
 * assembly mnemonic column aligned across all native instructions. */
#define HEX_WIDTH 32

static const char *jit_op_names[] = {
#define X(name, fmt, str) str,
#include "opcode_list.h"
#undef X
};

static const OpFormat jit_op_formats[] = {
#define X(name, fmt, str) fmt,
#include "opcode_list.h"
#undef X
};

/* Decode one bytecode word into an IR-style instruction text. */
static void format_insn_text(uint32_t insn, uint32_t payload, int pc, char *out,
                             size_t outsz)
{
    OpCode op = (OpCode)(insn >> 24);
    OpFormat fmt = jit_op_formats[op];
    const char *nm = jit_op_names[op];
    int rd, rs, rt, imm, ti, off, target;

    switch (fmt) {
        case FORMAT_RRR:
            rd = (insn >> 16) & 0xFF;
            rs = (insn >> 8) & 0xFF;
            rt = insn & 0xFF;
            snprintf(out, outsz, "r%d = %s r%d, r%d", rd, nm, rs, rt);
            break;

        case FORMAT_RRImm:
            rd = (insn >> 16) & 0xFF;
            rs = (insn >> 8) & 0xFF;
            imm = (int8_t)(insn & 0xFF);
            snprintf(out, outsz, "r%d = %s r%d, #%d", rd, nm, rs, imm);
            break;

        case FORMAT_RxRx:
            rd = (insn >> 12) & 0xFFF;
            rs = insn & 0xFFF;
            snprintf(out, outsz, "r%d = %s r%d", rd, nm, rs);
            break;

        case FORMAT_R_TI_Imm12:
            rd = (insn >> 16) & 0xFF;
            ti = (insn >> 12) & 0xF;
            imm = ((int)((insn & 0xFFF) << 20)) >> 20;
            snprintf(out, outsz, "r%d = %s #%d, ti=0x%x", rd, nm, imm, ti);
            break;

        case FORMAT_Rx:
            rs = insn & 0xFFF;
            snprintf(out, outsz, "%s r%d", nm, rs);
            break;

        case FORMAT_TI_Imm2:
            ti = (insn >> 16) & 0xFF;
            imm = (int16_t)(insn & 0xFFFF);
            snprintf(out, outsz, "%s #%d, ti=0x%x", nm, imm, ti);
            break;

        case FORMAT_Op:
            snprintf(out, outsz, "%s", nm);
            break;

        case FORMAT_JMP:
            off = (int16_t)(insn & 0xFFFF);
            target = pc + 1 + off;
            snprintf(out, outsz, "%s label %%bb%d", nm, target);
            break;

        case FORMAT_ROff2:
            rs = (insn >> 16) & 0xFF;
            off = (int16_t)(insn & 0xFFFF);
            target = pc + 1 + off;
            snprintf(out, outsz, "%s r%d, label %%bb%d", nm, rs, target);
            break;

        case FORMAT_RROff:
            rs = (insn >> 16) & 0xFF;
            rt = (insn >> 8) & 0xFF;
            off = (int8_t)(insn & 0xFF);
            target = pc + 1 + off;
            snprintf(out, outsz, "%s r%d, r%d, label %%bb%d", nm, rs, rt, target);
            break;

        case FORMAT_RImmOff:
            rs = (insn >> 16) & 0xFF;
            imm = (int8_t)((insn >> 8) & 0xFF);
            off = (int8_t)(insn & 0xFF);
            target = pc + 1 + off;
            snprintf(out, outsz, "%s r%d, #%d, label %%bb%d", nm, rs, imm, target);
            break;

        case FORMAT_CALL: {
            int flg = (insn >> 20) & 0xF;
            rd = (insn >> 8) & 0xFFF;
            int nargs = insn & 0xFF;
            if (rd == 0xFFF)
                snprintf(out, outsz, "%s #%d, flag=%d, entry=%u", nm, nargs, flg,
                         payload);
            else
                snprintf(out, outsz, "r%d = %s #%d, flag=%d, entry=%u", rd, nm,
                         nargs, flg, payload);
            break;
        }

        default:
            snprintf(out, outsz, "%s <raw 0x%08x>", nm, insn);
            break;
    }
}

static int is_jump_fmt(OpFormat fmt)
{
    return fmt == FORMAT_JMP || fmt == FORMAT_ROff2 || fmt == FORMAT_RROff ||
           fmt == FORMAT_RImmOff;
}

static int jump_target(uint32_t insn, int pc, OpFormat fmt)
{
    int off;
    switch (fmt) {
        case FORMAT_JMP:
        case FORMAT_ROff2: off = (int16_t)(insn & 0xFFFF); break;
        case FORMAT_RROff:
        case FORMAT_RImmOff: off = (int8_t)(insn & 0xFF); break;
        default: return -1;
    }
    return pc + 1 + off;
}

static int is_ret_op(OpCode op)
{
    return op == OP_RET || op == OP_RET_INT_IMM || op == OP_RET_VOID;
}

/* One line per native instruction in [from, to): hex bytes + disassembly.
 * Branch targets are mapped back to the IR block label when they land on a
 * known pc, or annotated as the function entry / error epilogue. */
static void print_native_range(FILE *fp, const uint8_t *code, size_t from,
                               size_t to, const size_t *pc_map, int n,
                               size_t err_off)
{
    size_t cur = from;
    while (cur < to) {
        char asm_text[128];
        size_t jt = SIZE_MAX;
        size_t ilen =
            jit_disasm_one(code + cur, to - cur, cur, asm_text,
                           sizeof(asm_text), &jt);
        if (ilen > to - cur) ilen = to - cur;

        char hex[64];
        int hp = 0;
        for (size_t k = 0; k < ilen; k++)
            hp += snprintf(hex + hp, sizeof(hex) - hp, "%s%02x",
                           k ? " " : "", code[cur + k]);

        char note[40];
        note[0] = '\0';
        if (jt == 0) {
            /* Offset 0 is the function entry (prologue), e.g. a direct
             * self-call. Payload slots also have pc_map==0, so don't search. */
            snprintf(note, sizeof(note), "  ; -> func entry");
        } else if (jt == err_off) {
            snprintf(note, sizeof(note), "  ; -> error epilogue");
        } else if (jt != SIZE_MAX) {
            for (int t = 0; t < n; t++) {
                if (pc_map[t] == jt) {
                    snprintf(note, sizeof(note), "  ; -> %%bb%d", t);
                    break;
                }
            }
        }

        fprintf(fp, "%*s;;   %-*s  %s%s\n", NOTE_COL, "", HEX_WIDTH, hex,
                asm_text, note);
        cur += ilen ? ilen : 1;
    }
}

void jit_print_code(const char *name, const uint32_t *insns, int n,
                    const size_t *pc_map, const uint8_t *code, size_t code_len,
                    size_t err_off, FILE *fp)
{
    if (!name) name = "?";
    if (n <= 0) return;

    /* Block leaders: pc 0, every jump target, and every fall-through after a
     * jump or return. Mirrors the basic-block structure of the IR printer. */
    uint8_t *leaders = calloc(n, 1);
    leaders[0] = 1;
    for (int pc = 0; pc < n; pc++) {
        OpCode op = (OpCode)(insns[pc] >> 24);
        OpFormat fmt = jit_op_formats[op];
        if (fmt == FORMAT_CALL) { pc++; continue; } /* skip the payload word */
        if (is_jump_fmt(fmt)) {
            int t = jump_target(insns[pc], pc, fmt);
            if (t >= 0 && t < n) leaders[t] = 1;
            if (pc + 1 < n) leaders[pc + 1] = 1;
        } else if (is_ret_op(op)) {
            if (pc + 1 < n) leaders[pc + 1] = 1;
        }
    }

    fprintf(fp, "[jit] func @%s {  ;; %d insns, %zu bytes, prologue %zuB\n", name,
            n, code_len, pc_map[0]);

    for (int pc = 0; pc < n; pc++) {
        if (leaders[pc]) fprintf(fp, "  %%bb%d:\n", pc);

        OpFormat fmt = jit_op_formats[(OpCode)(insns[pc] >> 24)];
        int words = (fmt == FORMAT_CALL) ? 2 : 1;
        uint32_t payload = (words == 2 && pc + 1 < n) ? insns[pc + 1] : 0;

        char text[160];
        format_insn_text(insns[pc], payload, pc, text, sizeof(text));

        size_t off = pc_map[pc];
        size_t end = (pc + words < n) ? pc_map[pc + words] : err_off;
        size_t nbytes = end - off;

        fprintf(fp, "        %04d: %-*s ;; +0x%03zx %2zuB\n", pc, OP_WIDTH, text,
                off, nbytes);

        print_native_range(fp, code, off, end, pc_map, n, err_off);

        pc += words - 1; /* skip the call payload word */
    }

    fprintf(fp, "  ;; error epilogue:\n");
    print_native_range(fp, code, err_off, code_len, pc_map, n, err_off);

    fprintf(fp, "[jit] }\n");
    free(leaders);
}

void jit_print_bail(const char *name, int pc, uint32_t insn, const char *reason,
                    FILE *fp)
{
    if (!name) name = "?";
    if (pc >= 0) {
        OpCode op = (OpCode)(insn >> 24);
        fprintf(fp, "[jit] bail @%s: %s at pc %d (op '%s', insn 0x%08x)\n", name,
                reason, pc, jit_op_names[op], insn);
    } else {
        fprintf(fp, "[jit] bail @%s: %s\n", name, reason);
    }
}

#ifdef __cplusplus
}
#endif