/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

static int can_swap(KlrValue *val)
{
    TypeSpec *ts = val->ts;
    if (ts->kind == TYPE_STR) return 0;
    return 1;
}

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
                if (can_swap(lhs) && klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_LT) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_GE;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_LE) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_GT;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_GT) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_LE;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_GE) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    insn->code = OP_BINARY_LT;
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_NE) {
                KlrValue *lhs = insn_oper_value(insn, 0);
                KlrValue *rhs = insn_oper_value(insn, 1);
                if (klr_is_const(lhs) && !klr_is_const(rhs)) {
                    set_operand_at(insn, 0, rhs);
                    set_operand_at(insn, 1, lhs);
                    change = 1;
                }
            } else if (insn->code == OP_BINARY_EQ) {
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
