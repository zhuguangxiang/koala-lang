/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LOWERING_H_
#define _KOALA_LOWERING_H_

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LowerInsn (LIR): Lowered, linearized, machine-dependent instruction.
 *
 * This is the canonical representation used by:
 *   - Lowering (ISel + fusion + splitting)
 *   - Linear Scan Register Allocation (LSRA)
 *   - Offset patching
 *   - Final bytecode encoding
 *
 * It is intentionally simple and flat, so LSRA can scan and rewrite it efficiently.
 */
typedef struct _LowerInsn {
    /* Target VM opcode after instruction selection */
    OpCode code;

    /* 4-byte fixed-length encoding format */
    OpFormat format;

    /*
     * Register Slot Fields (Overloaded logic):
     * Pre-LSRA:  Stores the virtual register (vreg) index from the IR.
     * Post-LSRA: Overwritten by LSRA with the physical register slot (0-255 or 0-4095).
     *
     * rd = destination register
     * rs = source register 1
     * rt = source register 2
     */
    int rd;
    int rs;
    int rt;

    /* Immediate / offset / constant pool index / field index.
     * Meaning depends on opcode and format.
     */
    int imm;

    /*
     * Control-flow target:
     *   - Points to the IR basic block that this jump/branch targets.
     *   - Used during encode() to compute PC-relative offsets.
     */
    KlrBasicBlock *target_bb;

    /*
     * Linearized program counter (LIR index):
     *   - Assigned during lowering.
     *   - Used by LSRA to compute live intervals.
     *   - Used by encode() to compute jump offsets.
     */
    int pc;

    /*
     * Optional: the IR instruction that produced this LIR instruction.
     * Useful for debugging, tracing, and error reporting.
     */
    KlrInsn *origin;
} LowerInsn;

uint32_t encode_insn(LowerInsn *insn);

// emit helpers
LowerInsn *emit_new(OpCode code, KlrInsn *origin, KlrFunc *fn);
LowerInsn *emit_Ax(OpCode code, int rd, KlrInsn *origin, KlrFunc *fn);
LowerInsn *emit_AxBx(OpCode code, int rd, int rs, KlrInsn *origin, KlrFunc *fn);
LowerInsn *emit_ABC(OpCode code, int rd, int rs, int rt, KlrInsn *origin, KlrFunc *fn);
LowerInsn *emit_ABxx(OpCode code, int rd, int imm16, KlrInsn *origin, KlrFunc *fn);
LowerInsn *emit_const(int rd, KlrValue *v, KlrInsn *origin, KlrFunc *fn);

// submodules
void kl_lower_binary(KlrFunc *fn, KlrInsn *insn);
void kl_lower_unary(KlrFunc *fn, KlrInsn *insn);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOWERING_H_ */
