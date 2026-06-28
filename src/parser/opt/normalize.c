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
            if (insn->code == OP_BINARY_ADD) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_CMPLT) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_CMPGE;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_CMPLE) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_CMPGT;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_CMPGT) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_CMPLE;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_CMPGE) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_CMPLT;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_CMPNE) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_CMPEQ) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    change = 1;
                }
            } else if (insn->code == OP_IR_JMP_COND) {
                KlrValue *cond = insn_oper_value(insn, 0);
                // [1] is reserved for fused jump target.
                KlrValue *tgt1 = insn_oper_value(insn, 2);
                KlrValue *tgt2 = insn_oper_value(insn, 3);

                // branch cond, bb1, bb1 → jmp bb1
                if (tgt1 == tgt2) {
                    insn->code = OP_JMP;
                    insn->num_opers = 1;
                    set_operand_at(insn, 0, tgt1);
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
