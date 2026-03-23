/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "isel.h"
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

    { OP_BINARY_CMPEQ, OP_INT_CMPEQ, OP_INT_CMPEQ_IMM, 1, 1 },
    { OP_BINARY_CMPNE, OP_INT_CMPNE, OP_INT_CMPNE_IMM, 1, 1 },
    { OP_BINARY_CMPLT, OP_INT_CMPLT, OP_INT_CMPLT_IMM, 0, 1 },
    { OP_BINARY_CMPGT, OP_INT_CMPGT, OP_INT_CMPGT_IMM, 0, 1 },
    { OP_BINARY_CMPLE, OP_INT_CMPLE, OP_INT_CMPLE_IMM, 0, 1 },
    { OP_BINARY_CMPGE, OP_INT_CMPGE, OP_INT_CMPGE_IMM, 0, 1 },
};

static BinaryRule uint_rules[] = {
    { OP_BINARY_DIV, OP_UINT_DIV, OP_UINT_DIV_IMM, 0, 1 },
    { OP_BINARY_MOD, OP_UINT_MOD, OP_UINT_MOD_IMM, 0, 1 },
    // logical shift
    { OP_BINARY_SHR, OP_UINT_SHR, OP_UINT_SHR_IMM, 0, 1 },
    { OP_BINARY_CMPLT, OP_UINT_CMPLT, OP_UINT_CMPLT_IMM, 0, 1 },
    { OP_BINARY_CMPLE, OP_UINT_CMPLE, OP_UINT_CMPLE_IMM, 0, 1 },
    { OP_BINARY_CMPGT, OP_UINT_CMPGT, OP_UINT_CMPGT_IMM, 0, 1 },
    { OP_BINARY_CMPGE, OP_UINT_CMPGE, OP_UINT_CMPGE_IMM, 0, 1 },
};

static BinaryRule float_rules[] = {
    { OP_BINARY_ADD, OP_FLOAT_ADD, 0, 1, 0 },
    { OP_BINARY_SUB, OP_FLOAT_SUB, 0, 0, 0 },
    { OP_BINARY_MUL, OP_FLOAT_MUL, 0, 1, 0 },
    { OP_BINARY_DIV, OP_FLOAT_DIV, 0, 0, 0 },
    { OP_BINARY_MOD, OP_FLOAT_MOD, 0, 0, 0 },
    { OP_BINARY_CMPLT, OP_FLOAT_CMPL, 0, 0, 0 },
    { OP_BINARY_CMPEQ, OP_FLOAT_CMPL, 0, 0, 0 },
    { OP_BINARY_CMPLE, OP_FLOAT_CMPL, 0, 0, 0 },
    { OP_BINARY_CMPGT, OP_FLOAT_CMPG, 0, 0, 0 },
    { OP_BINARY_CMPGE, OP_FLOAT_CMPG, 0, 0, 0 },
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
        // for unsigned types, if no specific rule, try to find the signed
        // version
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
    KlrValue *local;
    KlrInsn *insn;

    if (imm >= INT16_MIN && imm <= INT16_MAX) {
        /* Small immediate:
         * OP_LOCAL
         * OP_CONST_INT_IMM.
         */
        local = klr_build_local(bldr, c->ts, "");
        insn = klr_build_const_int(bldr, local, (KlrValue *)c);

        /* raw operands for small-imm form */
        set_raw_oper_index(insn, 0, 0);
        set_raw_oper_imm(insn, 1, imm);
    } else {
        /* Large immediate: materialize via constant pool.
         * OP_LOCAL
         * OP_CONST_LOAD.
         */
        local = klr_build_local(bldr, c->ts, "");
        insn = klr_build_const_load(bldr, local, (KlrValue *)c);

        /* raw operands for load-const form */
        set_raw_oper_index(insn, 0, 0);
        set_raw_oper_imm(insn, 1, imm);
    }

    return local;
}

KlrValue *isel_materialize_const(KlrFunc *fn, KlrInsn *at, KlrConst *c)
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
        UNREACHABLE();
    }

    // commutative swap: imm op reg -> reg op imm
    if (R->commutative && c1 && !c2) {
        KlrValue *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
        set_operand_at(insn, 0, lhs);
        set_operand_at(insn, 1, rhs);
        c1 = 0;
        c2 = 1;
    }

    // reg op imm
    if (c2) {
        KlrConst *rc = (KlrConst *)rhs;

        if (R->allow_imm) {
            // 8-bit fast path (only applies to int/uint rules)
            // float never enters this block because float rules set allow_imm =
            // 0 reg op imm
            ASSERT(rc->which == CONST_INT || rc->which == CONST_UINT);

            if (rc->which == CONST_INT) {
                int64_t imm = rc->ival;
                if (imm >= INT8_MIN && imm <= INT8_MAX) {
                    insn->code = R->imm_op;
                    set_raw_oper_reg(insn, 0);
                    set_raw_oper_index(insn, 1, 0);
                    set_raw_oper_imm(insn, 2, imm);
                    return;
                }
            } else {
                uint64_t uimm = (uint64_t)rc->ival;
                if (uimm <= UINT8_MAX) {
                    insn->code = R->imm_op;
                    set_raw_oper_reg(insn, 0);
                    set_raw_oper_index(insn, 1, 0);
                    set_raw_oper_imm(insn, 2, uimm);
                    return;
                }
            }
        }

        // imm too large → materialize
        // - large int/uint comes here because it doesn't fit in the imm field
        // - float always comes here because float rules have allow_imm = 0
        // reg op reg
        KlrValue *v = isel_materialize_const(fn, insn, rc);
        insn->code = R->reg_op;
        set_operand_at(insn, 1, v);
        set_raw_oper_reg(insn, 0);
        set_raw_oper_index(insn, 1, 0);
        set_raw_oper_index(insn, 2, 1);
        return;
    }

    // reg op reg
    insn->code = R->reg_op;
    set_raw_oper_reg(insn, 0);
    set_raw_oper_index(insn, 1, 0);
    set_raw_oper_index(insn, 2, 1);
}

static inline int isel_is_binary(OpCode op)
{
    return (op >= OP_BINARY_ADD && op <= OP_BINARY_CMPGE);
}

static void isel_materialize_push_const(KlrBuilder *bldr, KlrConst *c)
{
    int which = c->which;

    if (which == CONST_INT) {
        int64_t imm = c->ival;
        if (imm >= INT16_MIN && imm <= INT16_MAX) {
            klr_build_push_int_imm(bldr, (KlrValue *)c);
        } else {
            klr_build_push_const(bldr, (KlrValue *)c);
        }
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
    } else if (which == CONST_BOOL) {
        klr_build_push_bool(bldr, (KlrValue *)c);
    } else if (which == CONST_STR) {
        klr_build_push_const(bldr, (KlrValue *)c);
    } else {
        NYI();
    }
}

static void isel_lower_call_arg(KlrBuilder *bldr, KlrValue *arg)
{
    if (klr_is_const(arg)) {
        KlrConst *c = (KlrConst *)arg;
        isel_materialize_push_const(bldr, c);
    } else {
        KlrInsn *insn = klr_build_push(bldr, arg);
        set_raw_oper_index(insn, 0, 0);
    }
}

static void isel_lower_call(KlrInsn *insn, KlrFunc *fn)
{
    int nargs = insn->num_opers;

    KlrBuilder bldr;
    klr_builder_before(&bldr, insn);

    for (int i = 1; i < nargs; i++) {
        KlrValue *arg = insn_oper_value(insn, i);
        isel_lower_call_arg(&bldr, arg);
    }

    for (int i = 1; i < nargs; i++) {
        clear_operand_at(insn, i);
    }

    insn->code = OP_CALL;
    insn->num_opers = 1;
    insn->num_args = nargs - 1;
    ASSERT(insn->num_args >= 0);
}

static void isel_lower_ret(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *ret = insn_oper_value(insn, 0);
    ASSERT(klr_is_insn(ret) || klr_is_param(ret) || klr_is_const(ret));
    if (klr_is_const(ret)) {
        KlrConst *c = (KlrConst *)ret;
        if (c->which == CONST_INT) {
            int64_t imm = c->ival;
            if (imm >= INT16_MIN && imm <= INT16_MAX) {
                insn->code = OP_RET_INT_IMM;
                set_raw_oper_imm(insn, 0, imm);
                return;
            }
        } else {
            NYI();
        }
    }

    set_raw_oper_index(insn, 0, 0);
}

static void isel_lower_move_const(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst));
    ASSERT(klr_is_const(src));

    KlrConst *c = (KlrConst *)src;

    KlrBuilder bldr;
    klr_builder_before(&bldr, insn);

    if (c->which == CONST_INT) {
        int64_t imm = c->ival;
        if (imm >= INT16_MIN && imm <= INT16_MAX) {
            /* Small immediate: use OP_CONST_INT_IMM. */
            insn->code = OP_CONST_INT_IMM;
            set_raw_oper_index(insn, 0, 0);
            set_raw_oper_imm(insn, 1, imm);
        } else {
            /* Large immediate: materialize via constant pool. */
            insn->code = OP_CONST_LOAD;
            /* raw operands for load-const form */
            set_raw_oper_index(insn, 0, 0);
            set_raw_oper_imm(insn, 1, imm);
        }
        return;
    }

    NYI();
}

static void isel_lower_move(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst));

    if (klr_is_const(src)) {
        isel_lower_move_const(insn, fn);
    } else {
        ASSERT(klr_is_insn(src) || klr_is_param(src));
        set_raw_oper_index(insn, 0, 0);
        set_raw_oper_index(insn, 1, 1);
    }
}

static int klr_do_isel(KlrFunc *fn, void *data)
{
    log_info("isel for func '%s'", fn->name);

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach_reverse(insn, bb) {
            log_info("isel for insn:");
            log_insn(insn);

            if (isel_is_binary(insn->code)) {
                isel_lower_binary(insn, fn);
                continue;
            }

            switch (insn->code) {
                case OP_IR_CALL: {
                    isel_lower_call(insn, fn);
                    break;
                }

                case OP_RET: {
                    isel_lower_ret(insn, fn);
                    break;
                }

                case OP_MOVE: {
                    isel_lower_move(insn, fn);
                    break;
                }

                // other patterns...
                default: {
                    break;
                }
            }
        }
    }

    return 0;
}

static KlrPass isel_pass = {
    .name = "isel-pass",
    .run = klr_do_isel,
};

void build_isel_pm(KlrPassManager *pm, int dump)
{
    pm_add_pass(pm, &isel_pass, dump);
}

#ifdef __cplusplus
}
#endif
