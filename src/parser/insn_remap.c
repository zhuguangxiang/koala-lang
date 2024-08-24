/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "mm.h"

/* codegen: instruction selection */

#ifdef __cplusplus
extern "C" {
#endif

/* select better target machine opcode instruction */

/*
OP_IR_STORE     -->>        OP_LOAD/OP_MOVE
OP_JMP_xxZ      -->>        OP_JMP_INT_xx
*/

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
        } else if (v == 2) {
            insn->code = OP_CONST_INT_2;
        } else if (v == 3) {
            insn->code = OP_CONST_INT_3;
        } else if (v == 4) {
            insn->code = OP_CONST_INT_4;
        } else if (v == 5) {
            insn->code = OP_CONST_INT_5;
        } else {
            if (v >= INT8_MIN && v <= INT8_MAX) {
                insn->code = OP_CONST_INT_IMM8;
            } else {
                insn->code = OP_CONST_LOAD;
            }
        }
    } else if (val->which == CONST_FLT) {
        double v = val->fval;
        if (v == 0) {
            insn->code = OP_CONST_FLOAT_0;
        } else if (v == 1) {
            insn->code = OP_CONST_FLOAT_1;
        } else if (v == 2) {
            insn->code = OP_CONST_FLOAT_2;
        } else if (v == 3) {
            insn->code = OP_CONST_FLOAT_3;
        } else {
            insn->code = OP_CONST_LOAD;
        }
    } else if (val->which == CONST_BOOL) {
        int v = val->bval;
        if (v) {
            insn->code = OP_CONST_INT_1;
        } else {
            insn->code = OP_CONST_INT_0;
        }
    } else if (val->which == CONST_STR) {
        insn->code = OP_CONST_LOAD;
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
        new_op = OP_JMP_INT_CMP_EQ_IMM8 + (op - OP_BINARY_CMP_EQ);
    } else {
        new_op = OP_JMP_INT_CMP_EQ + (op - OP_BINARY_CMP_EQ);
    }
    return new_op;
}

static void try_combine_branch(KlrInsn *cond, KlrInsn *insn, KlrBasicBlock *bb)
{
    KlrValue *lhs = insn_operand_value(cond, 0);
    KlrValue *rhs = insn_operand_value(cond, 1);
    if (desc_is_int(lhs->desc)) {
        OpCode code = cond->code;
        if (rhs->kind == KLR_VALUE_CONST) {
            KlrConst *const_value = (KlrConst *)rhs;
            int64_t val = const_value->ival;
            if ((val >= INT8_MIN) && (val <= INT8_MAX)) {
                // remap to OP_JMP_INT_CMP_xx_IMM8
                code = get_jmp_cmp_int_op(code, 1);
                insn->code = code;
                update_insn_operand(insn, 0, lhs);
                update_insn_operand(insn, 1, rhs);
                return;
            }
        }

        // remap to OP_JMP_INT_CMP_xx
        code = get_jmp_cmp_int_op(code, 0);
        insn->code = code;
        update_insn_operand(insn, 0, lhs);
        update_insn_operand(insn, 1, rhs);
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
        NIY();
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
                default: {
                    // NIY();
                    break;
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
