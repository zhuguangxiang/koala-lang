/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "mm.h"

/* codegen: target instruction(byte code) selection */

#ifdef __cplusplus
extern "C" {
#endif

/* select better target machine opcode instruction */

static void remap_const_store(KlrOper *oper, KlrInsn *insn)
{
    KlrConst *val = (KlrConst *)oper->use.ref;
    if (val->which == CONST_INT) {
        int64_t v = val->ival;
        if (v == -1) {
            insn->code = OP_CONST_INT_M1;
        } else if (v == 0) {
            insn->code = OP_CONST_INT_0;
        } else if (v == 1) {
            insn->code = OP_CONST_INT_1;
        } else {
            if (v >= INT16_MIN && v <= INT16_MAX) {
                insn->code = OP_CONST_INT_IMM;
            } else {
                insn->code = OP_CONST;
            }
        }
    } else if (val->which == CONST_FLT) {
        double v = val->fval;
        if (v == 0) {
            insn->code = OP_CONST_FLOAT_0;
        } else {
            insn->code = OP_CONST;
        }
    } else if (val->which == CONST_BOOL) {
        int v = val->bval;
        if (v) {
            insn->code = OP_CONST_INT_1;
        } else {
            insn->code = OP_CONST_INT_0;
        }
    } else if (val->which == CONST_STR) {
        insn->code = OP_CONST;
    } else {
        UNREACHABLE();
    }
}

static void remap_ir_store(KlrInsn *insn, KlrBasicBlock *bb)
{
    KlrOper *oper = insn_operand(insn, 1);
    if (oper->kind == KLR_OPER_CONST) {
        remap_const_store(oper, insn);
    } else {
        // The 'move' opcode needs two registers.
        KlrOperKind kind = oper->kind;
        ASSERT(kind == KLR_OPER_PARAM || kind == KLR_OPER_LOCAL || kind == KLR_OPER_INSN);
        insn->code = OP_MOVE;
    }
}

static OpCode get_jmp_cmp_int_op(OpCode op, int imm)
{
    ASSERT((op >= OP_BINARY_CMP_EQ) && (op <= OP_BINARY_CMP_GE));
    OpCode new_op;
    if (imm) {
        new_op = OP_JMP_INT_CMP_EQ_IMM + (op - OP_BINARY_CMP_EQ);
    } else {
        new_op = OP_JMP_INT_CMP_EQ + (op - OP_BINARY_CMP_EQ);
    }
    return new_op;
}

static void try_combine_branch(KlrInsn *cond, KlrInsn *insn, KlrBasicBlock *bb)
{
    KlrValue *lhs = insn_oper_value(cond, 0);
    KlrValue *rhs = insn_oper_value(cond, 1);
    if (lhs->ts->kind == TYPE_INT) {
        OpCode code = cond->code;
        if (rhs->kind == KLR_VALUE_CONST) {
            KlrConst *const_value = (KlrConst *)rhs;
            int64_t val = const_value->ival;
            if ((val >= INT8_MIN) && (val <= INT8_MAX)) {
                // remap to OP_JMP_INT_CMP_xx_IMM8
                code = get_jmp_cmp_int_op(code, 1);
                insn->code = code;
                update_index_operand(insn, 0, lhs);
                update_index_operand(insn, 1, rhs);
                return;
            }
        }

        // remap to OP_JMP_INT_CMP_xx
        code = get_jmp_cmp_int_op(code, 0);
        insn->code = code;
        update_index_operand(insn, 0, lhs);
        update_index_operand(insn, 1, rhs);
    }
}

static void remap_ir_branch(KlrInsn *insn, KlrBasicBlock *bb)
{
    KlrOper *oper = insn_operand(insn, 0);
    if (oper->kind == KLR_OPER_INSN) {
        KlrInsn *cond = (KlrInsn *)oper->use.ref;
        ASSERT((cond->code >= OP_BINARY_CMP_EQ) && (cond->code <= OP_BINARY_CMP_GE));
        try_combine_branch(cond, insn, bb);
    } else {
        NYI();
    }
}

static void remap_ir_call(KlrInsn *insn, KlrBasicBlock *bb)
{
    KlrBuilder bldr;
    klr_builder_before(&bldr, insn);

    KlrValue *val;
    for (int i = 1; i < insn->num_opers; i++) {
        val = insn_oper_value(insn, i);
        KlrInsn *push_insn = klr_new_push(val);
        klr_append_insn(&bldr, push_insn);
    }
}

void klr_insn_remap(KlrFunc *func)
{
    KlrBasicBlock *bb;
    KlrInsn *insn, *nxt_insn;
    basic_block_foreach(bb, func) {
        insn_foreach_safe(insn, nxt_insn, bb) {
            switch (insn->code) {
                case OP_IR_STORE: {
                    remap_ir_store(insn, bb);
                    break;
                }
                case OP_IR_JMP_COND: {
                    remap_ir_branch(insn, bb);
                    break;
                }
                case OP_CALL: {
                    remap_ir_call(insn, bb);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
