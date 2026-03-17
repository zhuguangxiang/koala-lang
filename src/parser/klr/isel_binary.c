/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "ir.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static KlrValue *isel_build_int_literal(KlrBuilder *bldr, KlrConst *c)
{
    ASSERT(c->which == CONST_INT);
    int64_t imm = c->ival;
    KlrInsn *v;

    if (imm >= (int)(-0xFFF) && imm <= (int)(0xFFF)) {
        // small imm, can be encoded directly in the instruction
        v = klr_build_int_imm(bldr, c);
    } else {
        // large imm, materialize it as a register first
        v = klr_build_loadk(bldr, c);
    }

    return v;
}

static KlrValue *isel_build_float_literal(KlrBuilder *bldr, KlrConst *c)
{
    ASSERT(c->which == CONST_FLT);
    double v = c->fval;

    // 1. +0.0
    if (v == 0.0 && !signbit(v)) {
        insn = klr_builder_emit(bldr, OP_LOADK_SPECIAL);
        insn->dst = dst;
        insn->special = K_FLOAT_POS_ZERO;
        return dst;
    }

    // 2. -0.0
    if (v == 0.0 && signbit(v)) {
        insn = klr_builder_emit(bldr, OP_LOADK_SPECIAL);
        insn->dst = dst;
        insn->special = K_FLOAT_NEG_ZERO;
        return dst;
    }

    // 3. NaN
    if (isnan(v)) {
        insn = klr_builder_emit(bldr, OP_LOADK_SPECIAL);
        insn->dst = dst;
        insn->special = K_FLOAT_NAN;
        return dst;
    }

    // 4. +inf
    if (isinf(v) && v > 0) {
        insn = klr_builder_emit(bldr, OP_LOADK_SPECIAL);
        insn->dst = dst;
        insn->special = K_FLOAT_POS_INF;
        return dst;
    }

    // 5. -inf
    if (isinf(v) && v < 0) {
        insn = klr_builder_emit(bldr, OP_LOADK_SPECIAL);
        insn->dst = dst;
        insn->special = K_FLOAT_NEG_INF;
        return dst;
    }

    // 6. 普通浮点数 → 常量池
    int idx = klr_module_add_const(bldr->fn->module, c);
    insn = klr_builder_emit(bldr, OP_LOADK);
    insn->dst = dst;
    insn->kidx = idx;
    return dst;
}
/*
Binary lowering rule table
*/

typedef struct {
    int imm_opcode; // reg op imm
    int reg_opcode; // reg op reg
    bool commutative; // can swap lhs/rhs when lhs is const
    bool allow_imm; // whether IMM form is allowed
} IntBinRule;

KlrValue *isel_materialize_const_before(KlrFunc *fn, KlrInsn *at, KlrConst *c)
{
    KlrModule *m = fn->moddule;

    KlrBuilder bldr;
    klr_builder_before(&bldr, at);

    // materialize a constant into a register, and return the value
    int which = c->which;
    if (which == CONST_INT) {
        return isel_build_int_literal(&bldr, c);
    } else if (which == CONST_FLT) {
        return isel_build_float_literal(&bldr, c);
    } else if (which == CONST_BOOL) {
        return isel_build_bool_literal(&bldr, c);
    } else if (which == CONST_STR) {
        return isel_build_str_literal(&bldr, c);
    } else if (which == CONST_LIST) {
    } else if (which == CONST_TUPLE) {
    } else if (which == CONST_NONE) {
        return isel_build_none_literal(&bldr, c);
    } else {
        UNREACHABLE();
    }
}

static void isel_lower_int_binary(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *lval = insn_oper_value(insn, 0);
    KlrValue *rval = insn_oper_value(insn, 1);

    int c1 = klr_is_const(lval);
    int c2 = klr_is_const(rval);

    // helper: constant folding
    if (c1 && c2) {
        // imm op imm, constant folding should be done in optimizer, but we can
        // still do it here as a guarantee, since opt maybe not enabled.
        NYI();
    }

    // helper: swap for commutative ops
    if (c1 && !c2) {
        // imm op reg -> reg op imm
        KlrValue *tmp = lval;
        lval = rval;
        rval = tmp;
        c1 = 0;
        c2 = 1;
        update_index_operand(insn, 0, lval);
        update_index_operand(insn, 1, rval);
    }

    switch (insn->code) {
        case OP_BINARY_ADD: {
            if (c2) {
                // reg + imm
                KlrConst *rc = (KlrConst *)rval;
                ASSERT(rc->which == CONST_INT);
                int64_t imm = rc->ival;

                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = OP_INT_ADD_IMM;
                    break;
                }

                // imm is too large, materialize it as a register first
                KlrValue *_v = isel_materialize_const_before(fn, insn, rc);

                // change to reg + reg
                insn->code = OP_INT_ADD;
                update_index_operand(insn, 1, _v);
                break;
            }

            // reg + reg
            insn->code = OP_INT_ADD;
            break;
        }
        case OP_BINARY_SUB:
            if (c2) {
                // reg - imm
                KlrConst *rc = (KlrConst *)rval;
                ASSERT(rc->which == CONST_INT);
                int64_t imm = rc->ival;

                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = OP_INT_SUB_IMM;
                    break;
                }

                // imm is too large, materialize it as a register first
                KlrValue *_v = isel_materialize_const_before(fn, insn, rc);

                // change to reg - reg
                insn->code = OP_INT_SUB;
                update_index_operand(insn, 1, _v);
                break;
            }

            // reg - reg
            insn->code = OP_INT_SUB;
            break;
        case OP_BINARY_MUL:
            if (c2) {
                // reg * imm
                KlrConst *rc = (KlrConst *)rval;
                ASSERT(rc->which == CONST_INT);
                int64_t imm = rc->ival;

                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = OP_INT_MUL_IMM;
                    break;
                }

                // imm is too large, materialize it as a register first
                KlrValue *_v = isel_materialize_const_before(fn, insn, rc);

                // change to reg * reg
                insn->code = OP_INT_MUL;
                update_index_operand(insn, 1, _v);
                break;
            }

            // reg * reg
            insn->code = OP_INT_MUL;
            break;
        case OP_BINARY_DIV:
            if (c2) {
                // reg / imm
                KlrConst *rc = (KlrConst *)rval;
                ASSERT(rc->which == CONST_INT);
                int64_t imm = rc->ival;

                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = OP_INT_DIV_IMM;
                    break;
                }

                // imm is too large, materialize it as a register first
                KlrValue *_v = isel_materialize_const_before(fn, insn, rc);

                // change to reg / reg
                insn->code = OP_INT_DIV;
                update_index_operand(insn, 1, _v);
                break;
            }

            // reg / reg
            insn->code = OP_INT_DIV;
            break;

        case OP_BINARY_MOD:
            if (c2) {
                // reg % imm
                KlrConst *rc = (KlrConst *)rval;
                ASSERT(rc->which == CONST_INT);
                int64_t imm = rc->ival;

                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = OP_INT_MOD_IMM;
                    break;
                }

                // imm is too large, materialize it as a register first
                KlrValue *_v = isel_materialize_const_before(fn, insn, rc);

                // change to reg % reg
                insn->code = OP_INT_MOD;
                update_index_operand(insn, 1, _v);
                break;
            }

            // reg % reg
            insn->code = OP_INT_MOD;
            break;

        case OP_BINARY_CMP_EQ:
            if (c2) {
                // reg == imm
                KlrConst *rc = (KlrConst *)rval;
                ASSERT(rc->which == CONST_INT);
                int64_t imm = rc->ival;

                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = OP_INT_CMP_EQ_IMM;
                    break;
                }

                // imm is too large, materialize it as a register first
                KlrValue *_v = isel_materialize_const_before(fn, insn, rc);

                // change to reg == reg
                insn->code = OP_INT_CMP_EQ;
                update_index_operand(insn, 1, _v);
                break;
            }

            // reg == reg
            insn->code = OP_INT_CMP_EQ;
            break;
        case OP_BINARY_CMP_NE:
            insn->code = OP_INT_CMP_NE;
            break;
        case OP_BINARY_CMP_LT:
            insn->code = OP_INT_CMP_LT;
            break;
        case OP_BINARY_CMP_GT:
            insn->code = OP_INT_CMP_GT;
            break;
        case OP_BINARY_CMP_LE:
            insn->code = OP_INT_CMP_LE;
            break;
        case OP_BINARY_CMP_GE:
            insn->code = OP_INT_CMP_GE;
            break;
        default: {
            UNREACHABLE();
            break;
        }
    }
}

void isel_lower_binary(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *lval = insn_oper_value(insn, 0);
    KlrValue *rval = insn_oper_value(insn, 1);

    if (type_is_int(lval->ts) && type_is_int(rval->ts)) {
        isel_lower_int_binary(insn, fn);
        return;
    }

    if (type_is_float(lval->ts) && type_is_float(rval->ts)) {
        isel_lower_float_binary(insn, fn);
        return;
    }

    // fallback: generic binary op

    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);

    int c1 = klr_is_const(lval);
    int c2 = klr_is_const(rval);

    // constant folding hook (optional)
    if (c1 && c2) {
        // TODO: constant fold here if you want a safety net
        // NYI();
        return;
    }

    // commutative swap: imm op reg -> reg op imm
    if (R->commutative && c1 && !c2) {
        KlrValue *tmp = lval;
        lval = rval;
        rval = tmp;
        update_index_operand(insn, 0, lval);
        update_index_operand(insn, 1, rval);
        c1 = 0;
        c2 = 1;
    }

    // reg op imm
    if (c2 && R->allow_imm) {
        KlrConst *rc = (KlrConst *)rval;
        ASSERT(rc->which == CONST_INT || rc->which == CONST_UINT);

        if (rc->which == CONST_INT) {
            int64_t imm = rc->ival;
            if (imm >= INT8_MIN && imm <= INT8_MAX) {
                insn->code = R->imm_op;
                return;
            }
        } else {
            uint64_t uimm = rc->ival;
            if (uimm <= UINT8_MAX) {
                insn->code = R->imm_op;
                return;
            }
        }

        // imm too large → materialize
        KlrValue *v = isel_materialize_const_before(fn, insn, rc);
        insn->code = R->reg_opcode;
        update_index_operand(insn, 1, v);
        return;
    }

    // reg op reg
    insn->code = R->reg_opcode;
}

#ifdef __cplusplus
}
#endif
