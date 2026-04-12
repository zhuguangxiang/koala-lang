/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

int klr_normalize_pass(KlrFunc *fn, void *data)
{
    int change = 0;

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn->code == OP_BINARY_CMPLT) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_CMPGE;
                    change = 1;
                }
            }
        }
    }
    return change;
}

#ifdef __cplusplus
}
#endif
