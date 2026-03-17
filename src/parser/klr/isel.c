/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BinaryRule {
    // ir opcode
    OpCode ir_op;
    // reg-reg
    OpCode reg_op;
    // reg-imm
    OpCode imm_op;
    // whether this op is commutative (can swap lhs/rhs when lhs is const)
    int commutative;
    // whether imm form is allowed
    int allow_imm;
} BinaryRule;

// clang-format off

static BinaryRule int_rules[] = {
    { OP_BINARY_ADD, OP_INT_ADD, OP_INT_ADD_IMM, 1, 1 },
    { OP_BINARY_SUB, OP_INT_SUB, OP_INT_SUB_IMM, 0, 1 },
    { OP_BINARY_MUL, OP_INT_MUL, OP_INT_MUL_IMM, 1, 1 },
    { OP_BINARY_DIV, OP_INT_DIV, OP_INT_DIV_IMM, 0, 1 },
    { OP_BINARY_MOD, OP_INT_MOD, OP_INT_MOD_IMM, 0, 1 },

    { OP_BINARY_AND, OP_INT_AND, OP_INT_AND_IMM, 1, 1 },
    { OP_BINARY_OR, OP_INT_OR, OP_INT_OR_IMM, 1, 1 },
    { OP_BINARY_XOR, OP_INT_XOR, OP_INT_XOR_IMM, 1, 1 },
    { OP_BINARY_SHL, OP_INT_SHL, OP_INT_SHL_IMM, 0, 1 },
    // arithmetic shift
    { OP_BINARY_SHR, OP_INT_SHR, OP_INT_SHR_IMM, 0, 1 },

    { OP_BINARY_CMP_EQ, OP_INT_CMP_EQ, OP_INT_CMP_EQ_IMM, 1, 1 },
    { OP_BINARY_CMP_NE, OP_INT_CMP_NE, OP_INT_CMP_NE_IMM, 1, 1 },
    { OP_BINARY_CMP_LT, OP_INT_CMP_LT, OP_INT_CMP_LT_IMM, 0, 1 },
    { OP_BINARY_CMP_GT, OP_INT_CMP_GT, OP_INT_CMP_GT_IMM, 0, 1 },
    { OP_BINARY_CMP_LE, OP_INT_CMP_LE, OP_INT_CMP_LE_IMM, 0, 1 },
    { OP_BINARY_CMP_GE, OP_INT_CMP_GE, OP_INT_CMP_GE_IMM, 0, 1 },
};

static BinaryRule uint_rules[] = {
    { OP_BINARY_DIV, OP_UINT_DIV, OP_UINT_DIV_IMM, 0, 1 },
    { OP_BINARY_MOD, OP_UINT_MOD, OP_UINT_MOD_IMM, 0, 1 },
    // logical shift
    { OP_BINARY_SHR, OP_UINT_SHR, OP_UINT_SHR_IMM, 0, 1 },
    { OP_BINARY_CMP_LT, OP_UINT_CMP_LT, OP_UINT_CMP_LT_IMM, 0, 1 },
    { OP_BINARY_CMP_LE, OP_UINT_CMP_LE, OP_UINT_CMP_LE_IMM, 0, 1 },
    { OP_BINARY_CMP_GT, OP_UINT_CMP_GT, OP_UINT_CMP_GT_IMM, 0, 1 },
    { OP_BINARY_CMP_GE, OP_UINT_CMP_GE, OP_UINT_CMP_GE_IMM, 0, 1 },
};

static BinaryRule float_rules[] = {
    { OP_BINARY_ADD, OP_FLOAT_ADD, 0, 1, 0 },
    { OP_BINARY_SUB, OP_FLOAT_SUB, 0, 0, 0 },
    { OP_BINARY_MUL, OP_FLOAT_MUL, 0, 1, 0 },
    { OP_BINARY_DIV, OP_FLOAT_DIV, 0, 0, 0 },
    { OP_BINARY_MOD, OP_FLOAT_MOD, 0, 0, 0 },
    { OP_BINARY_CMP_LT, OP_FLOAT_CMPL, 0, 0, 0 },
    { OP_BINARY_CMP_EQ, OP_FLOAT_CMPL, 0, 0, 0 },
    { OP_BINARY_CMP_LE, OP_FLOAT_CMPL, 0, 0, 0 },
    { OP_BINARY_CMP_GT, OP_FLOAT_CMPG, 0, 0, 0 },
    { OP_BINARY_CMP_GE, OP_FLOAT_CMPG, 0, 0, 0 },
};

// clang-format on

BinaryRule *find_binary_rule(OpCode ir_op, TypeSpec *ts)
{
    BinaryRule *rules = NULL;
    int num_rules = 0;

    if (type_is_int(ts)) {
        rules = int_rules;
        num_rules = COUNT_OF(int_rules);
    } else if (type_is_uint(ts)) {
        rules = uint_rules;
        num_rules = COUNT_OF(uint_rules);
    } else if (type_is_float(ts)) {
        rules = float_rules;
        num_rules = COUNT_OF(float_rules);
    } else {
        return NULL;
    }

    for (int i = 0; i < num_rules; i++) {
        if (rules[i].ir_op == ir_op) {
            return (BinaryRule *)&rules[i];
        }
    }

    if (type_is_uint(ts)) {
        num_rules = COUNT_OF(int_rules);
        // for unsigned types, if no specific rule, try to find the signed version
        for (int i = 0; i < num_rules; i++) {
            if (int_rules[i].ir_op == ir_op) {
                return (BinaryRule *)&int_rules[i];
            }
        }
    }

    return NULL;
}

static KlrValue *isel_build_int_literal(KlrBuilder *bldr, KlrConst *c)
{
    ASSERT(c->which == CONST_INT);
    int64_t imm = c->ival;
    KlrValue *v;

    if (imm >= (int)(-0xFFF) && imm <= (int)(0xFFF)) {
        // small imm, can be encoded directly in the instruction
        v = klr_build_int_imm(bldr, (KlrValue *)c);
    } else {
        // large imm, materialize it as a register first
        v = klr_build_loadk(bldr, (KlrValue *)c);
    }

    return v;
}

KlrValue *isel_materialize_const_before(KlrFunc *fn, KlrInsn *at, KlrConst *c)
{
    KlrModule *m = fn->module;

    KlrBuilder bldr;
    klr_builder_before(&bldr, at);

    // materialize a constant into a register, and return the value
    int which = c->which;

    if (which == CONST_INT) {
        return isel_build_int_literal(&bldr, c);
        // } else if (which == CONST_FLT) {
        //     return isel_build_float_literal(&bldr, c);
        // } else if (which == CONST_BOOL) {
        //     return isel_build_bool_literal(&bldr, c);
        // } else if (which == CONST_STR) {
        //     return isel_build_str_literal(&bldr, c);
        // } else if (which == CONST_LIST) {
        // } else if (which == CONST_TUPLE) {
        // } else if (which == CONST_NONE) {
        //     return isel_build_none_literal(&bldr, c);
        // } else {
    } else {
        UNREACHABLE();
    }
}

static void isel_lower_binary(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);

    BinaryRule *R = find_binary_rule(insn->code, lhs->ts);
    ASSERT(R);

    int c1 = klr_is_const(lhs);
    int c2 = klr_is_const(rhs);

    // constant folding hook (optional)
    if (c1 && c2) {
        // TODO: constant fold here if you want a safety net
        // NYI();
        return;
    }

    // commutative swap: imm op reg -> reg op imm
    if (R->commutative && c1 && !c2) {
        KlrValue *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
        update_index_operand(insn, 0, lhs);
        update_index_operand(insn, 1, rhs);
        c1 = 0;
        c2 = 1;
    }

    // reg op imm
    if (c2) {
        KlrConst *rc = (KlrConst *)rhs;

        if (R->allow_imm) {
            // 8-bit fast path (only applies to int/uint rules)
            // float never enters this block because float rules set allow_imm = 0
            ASSERT(rc->which == CONST_INT || rc->which == CONST_UINT);

            if (rc->which == CONST_INT) {
                int64_t imm = rc->ival;
                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = R->imm_op;
                    return;
                }
            } else {
                uint64_t uimm = (uint64_t)rc->ival;
                if (uimm <= UINT8_MAX) {
                    insn->code = R->imm_op;
                    return;
                }
            }
        }

        // imm too large → materialize
        // - large int/uint comes here because it doesn't fit in the imm field
        // - float always comes here because float rules have allow_imm = 0
        KlrValue *v = isel_materialize_const_before(fn, insn, rc);
        insn->code = R->reg_op;
        update_index_operand(insn, 1, v);
        return;
    }

    // reg op reg
    insn->code = R->reg_op;
}

static inline int isel_is_binary(OpCode op)
{
    return (op >= OP_BINARY_ADD && op <= OP_BINARY_CMP_GE);
}

void klr_module_do_isel(KlrModule *m)
{
    log_info("do isel for module '%s'", m->name);

    KlrFunc *fn;
    vector_foreach(fn, &m->functions) {
        log_info("do isel for func '%s'", fn->name);
        KlrBasicBlock *bb;
        basic_block_foreach(bb, fn) {
            KlrInsn *insn;
            insn_foreach(insn, bb) {
                log_info("do isel for insn:");
                log_insn(insn);

                if (isel_is_binary(insn->code)) {
                    isel_lower_binary(insn, fn);
                    continue;
                }

                if (insn->code == OP_IR_LOCAL) {
                    // isel_lower_local(insn, fn);
                    continue;
                }

                // other isel patterns...
            }
        }
    }

    klr_dump_module(m);
}

#ifdef __cplusplus
}
#endif
