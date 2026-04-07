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
} LowerConstRule;

static KlrValue *lower_set_op_only(KlrConst *c, KlrInsn *insn, OpCode op)
{
    insn->code = op;
    return NULL;
}

static OpCode get_const_op(KlrConst *c, LowerConstRule *R)
{
    OpCode op = OP_NOP;
    switch (c->which) {
        case CONST_NONE: {
            op = R->tag_op;
            c->spec_tag = TAG_NONE;
            break;
        }

        case CONST_INT: {
            int64_t imm = c->ival;
            if (imm >= INT16_MIN && imm <= INT16_MAX) {
                op = R->imm_op;
            } else {
                op = R->load_op;
            }
            break;
        }

        case CONST_UINT: {
            uint64_t uimm = (uint64_t)c->ival;
            if (uimm <= UINT16_MAX) {
                op = R->imm_op;
            } else {
                op = R->load_op;
            }
            break;
        }

        case CONST_FLT: {
            double v = c->fval;
            if (v == 0.0) {
                op = R->tag_op;
                c->spec_tag = signbit(v) ? TAG_FLOAT_NEG_ZERO : TAG_FLOAT_POS_ZERO;
            } else if (isnan(v)) {
                op = R->tag_op;
                c->spec_tag = TAG_FLOAT_NAN;
            } else if (isinf(v)) {
                op = R->tag_op;
                c->spec_tag = signbit(v) ? TAG_FLOAT_NEG_INF : TAG_FLOAT_POS_INF;
            } else {
                op = R->load_op;
            }
            break;
        }

        case CONST_BOOL: {
            op = R->tag_op;
            c->spec_tag = c->bval ? TAG_BOOL_TRUE : TAG_BOOL_FALSE;
            break;
        }

        case CONST_STR: {
            op = R->load_op;
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }
    return op;
}

static KlrValue *lower_binary_const(KlrFunc *fn, KlrInsn *at, KlrConst *c)
{
    KlrBuilder bldr;
    klr_builder_before(&bldr, at);

    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK };
    OpCode op = get_const_op(c, &R);
    KlrValue *local = klr_build_local(&bldr, c->ts, "");
    klr_build_load(&bldr, local, (KlrValue *)c, op);
    return local;
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

// Lower a single call argument into a fixed call-slot.
// 'pos' is the slot index counted from the end of the frame.
static void lower_call_argument(KlrInsn *insn, KlrValue *arg, int pos)
{
    // Parameters are handled separately (ABI-specific).
    if (klr_is_param(arg)) {
        KlrBuilder bldr;
        klr_builder_before(&bldr, insn);

        KlrValue *local = klr_build_local_var(&bldr, arg->ts, "");
        KlrInsn *_insn = (KlrInsn *)local;

        _insn->fixedslot = 1;   // mark as fixed slot
        _insn->slotindex = pos; // assign slot index

        klr_build_move(&bldr, local, arg); // copy original value
        return;
    }

    // Constants require a dedicated lowering path.
    if (klr_is_const(arg)) {
        KlrBuilder bldr;
        klr_builder_before(&bldr, insn);

        KlrValue *local = klr_build_local_var(&bldr, arg->ts, "");
        KlrInsn *_insn = (KlrInsn *)local;

        _insn->fixedslot = 1;   // mark as fixed slot
        _insn->slotindex = pos; // assign slot index

        LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK };
        OpCode op = get_const_op((KlrConst *)arg, &R);
        klr_build_load(&bldr, local, arg, op);
        return;
    }

    // Generic SSA value: force it into a fixed call-slot.
    // If the value has a single use, reuse the defining instruction.
    if (arg->use_count == 1) {
        KlrInsn *_insn = (KlrInsn *)arg;
        _insn->fixedslot = 1;   // mark as fixed slot
        _insn->slotindex = pos; // assign slot index
        return;
    }

    // Multiple uses: create a dedicated local and move into it.
    KlrBuilder bldr;
    klr_builder_before(&bldr, insn);

    KlrValue *local = klr_build_local_var(&bldr, arg->ts, "");
    KlrInsn *_insn = (KlrInsn *)local;

    _insn->fixedslot = 1;   // mark as fixed slot
    _insn->slotindex = pos; // assign slot index

    klr_build_move(&bldr, local, arg); // copy original value
}

static int param_is_used(KlrValue *param, KlrValue *val)
{
    if (klr_is_const(val)) {
        return 0;
    }

    if (klr_is_param(val)) {
        return val == param;
    }

    KlrValue *_val;
    insn_oper_value_foreach(_val, (KlrInsn *)val) {
        if (_val == param) {
            return 1;
        }
    }

    return 0;
}

static int allow_fixslot(KlrValue *param, KlrValue *val, KlrInsn *call)
{
    int num_opers = call->num_opers;
    KlrUse *use;
    int i, j;
    for (i = 1; i < num_opers; i++) {
        use = &call->opers[i].use;
        if (use->ref == val) break;
    }

    for (j = i + 1; j < num_opers; j++) {
        use = &call->opers[j].use;
        if (param_is_used(param, use->ref)) {
            return 0;
        }
    }

    return 1;
}

static void lower_tailcall_fixslot(KlrValue *dst, KlrValue *src, int pos, KlrInsn *call,
                                   KlrBuilder *bldr)
{
    if (!klr_is_const(src)) {
        if (src->use_count == 1 && allow_fixslot(dst, src, call)) {
            KlrInsn *_insn = (KlrInsn *)src;
            _insn->fixedslot = 2;
            _insn->slotindex = pos;
            return;
        }

        klr_build_move(bldr, dst, src);
        return;
    }

    KlrConst *c = (KlrConst *)src;
    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK };
    OpCode op = get_const_op(c, &R);
    klr_build_load(bldr, dst, src, op);
}

static void lower_tailcall_move(KlrValue *dst, KlrValue *src, KlrBuilder *bldr)
{
    if (!klr_is_const(src)) {
        klr_build_move(bldr, dst, src);
        return;
    }

    KlrConst *c = (KlrConst *)src;
    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK };
    OpCode op = get_const_op(c, &R);
    klr_build_load(bldr, dst, src, op);
}

static void lower_tailcall_argument(KlrInsn *insn, KlrValue *arg, int index, KlrFunc *fn,
                                    KlrInsn **last_local)
{
    int nparams = vector_size(&fn->params);
    if (index < nparams) {
        KlrBuilder bldr;
        klr_builder_before(&bldr, insn);
        KlrValue *param = vector_get(&fn->params, index);
        lower_tailcall_fixslot(param, arg, index, insn, &bldr);
        log_info("tailcall argument %d -> param %s", index, param->name);
    } else {
        KlrBuilder bldr;

        if (*last_local) {
            klr_builder_at(&bldr, *last_local);
        } else {
            KlrBasicBlock *bb = first_basic_block(fn);
            klr_builder_head(&bldr, bb);
        }

        KlrValue *local = klr_build_local(&bldr, arg->ts, "");
        lower_tailcall_move(local, arg, &bldr);
        *last_local = (KlrInsn *)local;
        log_info(
            "tailcall arguments are more than parameters, argument %d will be moved "
            "to a local variable",
            index);
    }
}

static int is_tailcall(KlrInsn *insn)
{
    KlrBasicBlock *bb = insn->bb;
    KlrInsn *next = insn_next(insn, bb);
    if (!next || next->code != OP_RET) return 0;

    KlrValue *val = insn_oper_value(next, 0);
    if (val != (KlrValue *)insn) return 0;

    ASSERT(insn_last(bb) == next);
    klr_erase_insn(next);
    return 1;
}

static void isel_lower_call(KlrInsn *insn, KlrFunc *fn)
{
    if (tail_call_enabled() && is_tailcall(insn)) {
        fn->has_tailcall = 1;
        KlrInsn *last_local = NULL;
        int nargs = insn->num_opers;
        for (int i = 1; i < nargs; i++) {
            KlrValue *arg = insn_oper_value(insn, i);
            lower_tailcall_argument(insn, arg, i - 1, fn, &last_local);
        }

        for (int i = 1; i < nargs; i++) {
            clear_operand_at(insn, i);
        }

        insn->code = OP_TAIL_CALL;
        insn->num_opers = 1;
        insn->num_args = nargs - 1;
        ASSERT(insn->num_args >= 0);
        return;
    }

    int nargs = insn->num_opers;
    for (int i = 1; i < nargs; i++) {
        KlrValue *arg = insn_oper_value(insn, i);
        lower_call_argument(insn, arg, i - 1);
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
    LowerConstRule R = { OP_RET_INT_IMM, OP_RET_TAG, OP_RET_CONST };
    OpCode op = get_const_op(c, &R);
    insn->code = op;
}

static void isel_lower_move(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst) || klr_is_param(dst));
    ASSERT(klr_is_insn(src) || klr_is_param(src) || klr_is_const(src));

    if (!klr_is_const(src)) return;

    KlrConst *c = (KlrConst *)src;
    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK };
    OpCode op = get_const_op(c, &R);
    insn->code = op;
}

static inline int is_int_cmp(OpCode op)
{
    return (op >= OP_INT_CMPEQ) && (op <= OP_INT_CMPGE_IMM);
}

static OpCode int_cmp_map[] = {
    OP_JMP_INT_EQ, OP_JMP_INT_EQ_IMM, OP_JMP_INT_NE, OP_JMP_INT_NE_IMM,
    OP_JMP_INT_LT, OP_JMP_INT_LT_IMM, OP_JMP_INT_LE, OP_JMP_INT_LE_IMM,
    OP_JMP_INT_GT, OP_JMP_INT_GT_IMM, OP_JMP_INT_GE, OP_JMP_INT_GE_IMM,
};

static inline int is_uint_cmp(OpCode op)
{
    return ((op >= OP_UINT_CMPLT) && (op <= OP_UINT_CMPGE_IMM)) ||
           (op >= OP_INT_CMPEQ && op <= OP_INT_CMPNE_IMM);
}

static OpCode uint_cmp_map[] = {
    OP_JMP_UINT_LT, OP_JMP_UINT_LT_IMM, OP_JMP_UINT_LE, OP_JMP_UINT_LE_IMM,
    OP_JMP_UINT_GT, OP_JMP_UINT_GT_IMM, OP_JMP_UINT_GE, OP_JMP_UINT_GE_IMM,
};

static void isel_lower_jmp_cond(KlrInsn *insn, KlrFunc *fn)
{
    KlrBasicBlock *bb = insn->bb;
    KlrInsn *prev = insn_prev(insn, bb);
    if (!prev) return;
    if (prev->bb != bb) return;
    if (prev->use_count != 1) return;
    KlrValue *cond = insn_oper_value(insn, 0);
    if (cond != (KlrValue *)prev) return;

    TypeSpec *ts = prev->ts;
    ASSERT(type_is_bool(ts));

    if (is_int_cmp(prev->code)) {
        KlrValue *lhs = insn_oper_value(prev, 0);
        KlrValue *rhs = insn_oper_value(prev, 1);
        int idx = prev->code - OP_INT_CMPEQ;
        ASSERT(idx >= 0 && idx < COUNT_OF(int_cmp_map));
        insn->code = int_cmp_map[idx];
        set_operand_at(insn, 0, lhs);
        set_operand_at(insn, 1, rhs);
        klr_erase_insn(prev);
        return;
    }

    if (is_uint_cmp(prev->code)) {
        KlrValue *lhs = insn_oper_value(prev, 0);
        KlrValue *rhs = insn_oper_value(prev, 1);

        OpCode op = prev->code;
        if ((op >= OP_UINT_CMPLT) && (op <= OP_UINT_CMPGE_IMM)) {
            int idx = op - OP_UINT_CMPLT;
            ASSERT(idx >= 0 && idx < COUNT_OF(uint_cmp_map));
            insn->code = uint_cmp_map[idx];
        } else if ((op >= OP_INT_CMPEQ) && (op <= OP_INT_CMPNE_IMM)) {
            int idx = op - OP_INT_CMPEQ;
            ASSERT(idx >= 0 && idx < COUNT_OF(int_cmp_map));
            insn->code = int_cmp_map[idx];
        }

        set_operand_at(insn, 0, lhs);
        set_operand_at(insn, 1, rhs);
        klr_erase_insn(prev);
        return;
    }
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

    int max = 0;
    KlrBasicBlock *bb;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            verify_insn(insn);
        }
    }

    // get max call arguments
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn_is(insn, OP_IR_CALL)) {
                max = MAX(max, insn->num_opers - 1);
            }
        }
    }

    fn->max_call_args = max;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach_reverse(insn, bb) {
            if (insn->code != OP_IR_CALL) {
                continue;
            }
            isel_lower_call(insn, fn);
        }
    }

    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (isel_is_binary(insn->code)) {
                isel_lower_binary(insn, fn);
                continue;
            }

            switch (insn->code) {
                case OP_RET: {
                    isel_lower_ret(insn, fn);
                    break;
                }

                case OP_MOVE: {
                    isel_lower_move(insn, fn);
                    break;
                }

                case OP_IR_JMP_COND: {
                    if (fusion_enabled()) {
                        isel_lower_jmp_cond(insn, fn);
                    }
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
