/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CGEN_H_
#define _KOALA_CGEN_H_

#include "parser.h"
#include "pass.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * KlMachInsn(LIR): Lowered, linearized, machine-dependent instruction.
 *
 * This is the canonical representation used by:
 *   - Lowering (ISel + fusion + splitting)
 *   - Linear Scan Register Allocation (LSRA)
 *   - Offset patching
 *   - Final bytecode encoding
 *
 * It is intentionally simple and flat, so LSRA can scan and rewrite it efficiently.
 */
typedef struct _KlMachInsn {
    /* opcode after isel */
    OpCode code;

    /* 4-byte fixed-length encoding format */
    OpFormat format;

    /* Physical fields for encoding. */
    int A;
    int B;
    int C;

    int Ax;
    int Bx;

    int Axx;
    int Bxx;
    int Axxx;

    /* Branch targets (machine-level blocks). */
    struct _KlMachBlock *target_true;
    struct _KlMachBlock *target_false;

    /* Linearized instruction index (per function) */
    int pc;

    /* Debug: original IR instruction */
    KlrInsn *origin;
} KlMachInsn;

typedef struct _KlMachBlock {
    /* ->bb_list in KlMachFunc */
    List link;

    /* [start_pc, end_pc] */
    int start_pc;
    int end_pc;

    /* Vector<KlMachInsn *> */
    Vector insns;

    /* original basic block */
    KlrBasicBlock *origin;
} KlMachBlock;

typedef struct _KlMachFunc {
    /* original IR function */
    KlrFunc *origin;
    /* block list in layout order */
    List bb_list;
    /* total number of instructions */
    int total_insns;
} KlMachFunc;

KlMachFunc *klm_linearize_func(KlrFunc *fn);
void klm_dump_func(KlMachFunc *fn);

void build_cgen_pm(KlrPassManager *pm, int dump);

// uint32_t encode_insn(KlmInsn *insn);

// // emit helpers
// KlmInsn *emit_new(OpCode code, KlrInsn *origin, KlrFunc *fn);
// KlmInsn *emit_Ax(OpCode code, int rd, KlrInsn *origin, KlrFunc *fn);
// KlmInsn *emit_AxBx(OpCode code, int rd, int rs, KlrInsn *origin, KlrFunc *fn);
// KlmInsn *emit_ABC(OpCode code, int rd, int rs, int rt, KlrInsn *origin, KlrFunc *fn);
// KlmInsn *emit_ABxx(OpCode code, int rd, int imm16, KlrInsn *origin, KlrFunc *fn);
// KlmInsn *emit_const(int rd, KlrValue *v, KlrInsn *origin, KlrFunc *fn);

// // submodules
// void kl_lower_binary(KlrFunc *fn, KlrInsn *insn);
// void kl_lower_unary(KlrFunc *fn, KlrInsn *insn);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CGEN_H_ */
