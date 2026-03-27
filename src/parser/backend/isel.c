/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "cmd.h"
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

typedef struct _LowerConstRule {
    OpCode imm_op;
    OpCode tag_op;
    OpCode load_op;
    KlrValue *(*lower)(KlrConst *, KlrInsn *, OpCode);
} LowerConstRule;

static KlrValue *lower_set_op_only(KlrConst *c, KlrInsn *insn, OpCode op)
{
    insn->code = op;
    return NULL;
}

static KlrValue *do_lower_const(KlrConst *c, KlrInsn *insn, LowerConstRule *R)
{
    KlrValue *ret = NULL;

    switch (c->which) {
        case CONST_NONE: {
            ret = R->lower(c, insn, R->tag_op);
            c->spec_tag = TAG_NONE;
            break;
        }

        case CONST_INT: {
            int64_t imm = c->ival;
            if (imm >= INT16_MIN && imm <= INT16_MAX) {
                ret = R->lower(c, insn, R->imm_op);
            } else {
                ret = R->lower(c, insn, R->load_op);
            }
            break;
        }

        case CONST_UINT: {
            uint64_t uimm = (uint64_t)c->ival;
            if (uimm <= UINT16_MAX) {
                ret = R->lower(c, insn, R->imm_op);
            } else {
                ret = R->lower(c, insn, R->load_op);
            }
            break;
        }

        case CONST_FLT: {
            double v = c->fval;
            if (v == 0.0) {
                ret = R->lower(c, insn, R->tag_op);
                c->spec_tag = signbit(v) ? TAG_FLOAT_NEG_ZERO : TAG_FLOAT_POS_ZERO;
            } else if (isnan(v)) {
                ret = R->lower(c, insn, R->tag_op);
                c->spec_tag = TAG_FLOAT_NAN;
            } else if (isinf(v)) {
                ret = R->lower(c, insn, R->tag_op);
                c->spec_tag = signbit(v) ? TAG_FLOAT_NEG_INF : TAG_FLOAT_POS_INF;
            } else {
                ret = R->lower(c, insn, R->load_op);
            }
            break;
        }

        case CONST_BOOL: {
            ret = R->lower(c, insn, R->tag_op);
            c->spec_tag = c->bval ? TAG_BOOL_TRUE : TAG_BOOL_FALSE;
            break;
        }

        case CONST_STR: {
            ret = R->lower(c, insn, R->load_op);
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }

    return ret;
}

static KlrValue *lower_load_binary_const(KlrConst *c, KlrInsn *insn, OpCode op)
{
    KlrBuilder bldr;
    klr_builder_before(&bldr, insn);

#ifndef NDEBUG
    int which = c->which;
    if (op == OP_LOAD_INT_IMM) {
        ASSERT(which == CONST_INT || which == CONST_UINT);
    } else if (op == OP_LOADK) {
        ASSERT(which == CONST_INT || which == CONST_UINT || which == CONST_FLT);
    } else {
        ASSERT(op == OP_LOAD_TAG);
        ASSERT(which == CONST_FLT);
    }
#endif

    KlrValue *local = klr_build_local(&bldr, c->ts, "");
    klr_build_load(&bldr, local, (KlrValue *)c, op);
    return local;
}

static KlrValue *lower_binary_const(KlrFunc *fn, KlrInsn *at, KlrConst *c)
{
    KlrModule *m = fn->module;

    static LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK,
                                lower_load_binary_const };
    return do_lower_const(c, at, &R);
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
        // reg op reg
        KlrValue *v = lower_binary_const(fn, insn, rc);
        insn->code = R->reg_op;
        set_operand_at(insn, 1, v);
        return;
    }

    // reg op reg
    insn->code = R->reg_op;
}

static inline int isel_is_binary(OpCode op)
{
    return (op >= OP_BINARY_ADD && op <= OP_BINARY_CMPGE);
}

static KlrValue *lower_push_const(KlrConst *c, KlrInsn *insn, OpCode op)
{
    KlrBuilder bldr;
    klr_builder_before(&bldr, insn);
    return (KlrValue *)klr_build_push(&bldr, (KlrValue *)c, op);
}

static void lower_call_arguemnt(KlrInsn *insn, KlrValue *arg)
{
    if (!klr_is_const(arg)) {
        KlrBuilder bldr;
        klr_builder_before(&bldr, insn);
        klr_build_push(&bldr, arg, OP_PUSH);
        return;
    }

    KlrConst *c = (KlrConst *)arg;
    static LowerConstRule R = { OP_PUSH_INT_IMM, OP_PUSH_TAG, OP_PUSH_CONST,
                                lower_push_const };
    do_lower_const(c, insn, &R);
}

static void isel_lower_call(KlrInsn *insn, KlrFunc *fn)
{
    int nargs = insn->num_opers;

    for (int i = 1; i < nargs; i++) {
        KlrValue *arg = insn_oper_value(insn, i);
        lower_call_arguemnt(insn, arg);
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

    if (!klr_is_const(ret)) return;

    KlrConst *c = (KlrConst *)ret;
    static LowerConstRule R = { OP_RET_INT_IMM, OP_RET_TAG, OP_RET_CONST,
                                lower_set_op_only };
    do_lower_const(c, insn, &R);
}

static void isel_lower_move(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst));
    ASSERT(klr_is_insn(src) || klr_is_param(src) || klr_is_const(src));

    if (!klr_is_const(src)) return;

    KlrConst *c = (KlrConst *)src;
    static LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK,
                                lower_set_op_only };
    do_lower_const(c, insn, &R);
}

static void verify_insn(KlrInsn *insn)
{
    OpCode op = insn->code;

    if ((op >= OP_BINARY_ADD && op <= OP_IR_PHI) || (op == OP_JMP) || (op == OP_RET) ||
        (op == OP_RET_VOID) || (op == OP_MOVE) || (op == OP_GLOBAL_GET) ||
        (op == OP_GLOBAL_SET)) {
        return;
    }

    panic("unexpected opcode in isel input sequence: %s", op_name(op));
}

static void do_isel(KlrFunc *fn)
{
    log_info("isel for func '%s'", fn->name);

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            log_info("isel for insn:");
            log_insn(insn);
            verify_insn(insn);

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
}

void kl_do_isel(KlrModule *m)
{
    KlrFunc *fn;
    vector_foreach(fn, &m->functions) {
        do_isel(fn);
        if (dump_lir_enabled()) {
            fprintf(stdout, "--- IR Dump After isel [@%s] ---\n", fn->name);
            klr_print_func(fn, stdout);
        }
    }
}

#ifdef __cplusplus
}
#endif
