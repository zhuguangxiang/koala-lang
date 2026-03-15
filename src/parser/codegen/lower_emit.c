/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "lowering.h"

#ifdef __cplusplus
extern "C" {
#endif

LowerInsn *emit_new(OpCode code, KlrInsn *origin, KlrFunc *fn)
{
    LowerInsn insn = { 0 };

    insn.code = code;
    insn.origin = origin;
    insn.pc = vector_size(&fn->lir);
    vector_push_back(&fn->lir, &insn);

    return vector_last(&fn->lir);
}

LowerInsn *emit_Ax(OpCode code, int rd, KlrInsn *origin, KlrFunc *fn)
{
    LowerInsn *insn = emit_new(code, origin, fn);
    insn->format = FORMAT_Ax;
    insn->rd = rd;
    return insn;
}

LowerInsn *emit_AxBx(OpCode code, int rd, int rs, KlrInsn *origin, KlrFunc *fn)
{
    LowerInsn *insn = emit_new(code, origin, fn);
    insn->format = FORMAT_AxBx;
    insn->rd = rd;
    insn->rs = rs;
    return insn;
}

LowerInsn *emit_ABC(OpCode code, int rd, int rs, int rt, KlrInsn *origin, KlrFunc *fn)
{
    LowerInsn *insn = emit_new(code, origin, fn);
    insn->format = FORMAT_ABC;
    insn->rd = rd;
    insn->rs = rs;
    insn->rt = rt;
    return insn;
}

LowerInsn *emit_ABxx(OpCode code, int rd, int imm16, KlrInsn *origin, KlrFunc *fn)
{
    LowerInsn *insn = emit_new(code, origin, fn);
    insn->format = FORMAT_ABxx;
    insn->rd = rd;
    insn->imm = imm16;
    return insn;
}

LowerInsn *emit_const_int(int rd, int value, KlrInsn *origin, KlrFunc *fn)
{
    if (value == -1) {
        return emit_Ax(OP_CONST_INT_M1, rd, origin, fn);
    }

    if (value == 0) {
        return emit_Ax(OP_CONST_INT_0, rd, origin, fn);
    }

    if (value == 1) {
        return emit_Ax(OP_CONST_INT_1, rd, origin, fn);
    }

    if (value >= INT16_MIN && value <= INT16_MAX) {
        return emit_ABxx(OP_CONST_INT_IMM, rd, value, origin, fn);
    }

    NYI();
    return NULL;
}

#ifdef __cplusplus
}
#endif
