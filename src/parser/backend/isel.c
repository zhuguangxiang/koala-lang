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
    { OP_BINARY_ADD, OP_INT_ADD, OP_UINT_ADD_IMM, 1, 1 },
    { OP_BINARY_SUB, OP_INT_SUB, OP_UINT_SUB_IMM, 0, 1 },
    { OP_BINARY_MUL, OP_INT_MUL, OP_UINT_MUL_IMM, 1, 1 },
    { OP_BINARY_DIV, OP_UINT_DIV, OP_UINT_DIV_IMM, 0, 1 },
    { OP_BINARY_MOD, OP_UINT_MOD, OP_UINT_MOD_IMM, 0, 1 },

    { OP_BINARY_AND, OP_INT_AND, OP_UINT_AND_IMM, 1, 1 },
    { OP_BINARY_OR, OP_INT_OR, OP_UINT_OR_IMM, 1, 1 },
    { OP_BINARY_XOR, OP_INT_XOR, OP_UINT_XOR_IMM, 1, 1 },
    { OP_BINARY_SHL, OP_INT_SHL, OP_UINT_SHL_IMM, 0, 1 },
     // logical shift
    { OP_BINARY_SHR, OP_UINT_SHR, OP_UINT_SHR_IMM, 0, 1 },

    { OP_BINARY_CMPEQ, OP_INT_CMPEQ, OP_UINT_CMPEQ_IMM, 1, 1 },
    { OP_BINARY_CMPNE, OP_INT_CMPNE, OP_UINT_CMPNE_IMM, 1, 1 },
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
    { OP_BINARY_CMPNE, OP_FLOAT_CMPNE, 0, 0, 0 },
    { OP_BINARY_CMPLT, OP_FLOAT_CMPLT, 0, 0, 0 },
    { OP_BINARY_CMPEQ, OP_FLOAT_CMPEQ, 0, 0, 0 },
    { OP_BINARY_CMPLE, OP_FLOAT_CMPLE, 0, 0, 0 },
    { OP_BINARY_CMPGT, OP_FLOAT_CMPGT, 0, 0, 0 },
    { OP_BINARY_CMPGE, OP_FLOAT_CMPGE, 0, 0, 0 },
};

static BinaryRule optional_rules[] = {
    { OP_BINARY_CMPEQ, OP_REF_EQ, OP_REF_EQ_NULL, 1, 1 },
    { OP_BINARY_CMPNE, OP_REF_NE, OP_REF_NE_NULL, 1, 1 },
};

static BinaryRule bool_rules[] = {
    { OP_BINARY_CMPEQ, OP_INT_CMPEQ, OP_INT_CMPEQ_IMM, 1, 1 },
    { OP_BINARY_CMPNE, OP_INT_CMPNE, OP_INT_CMPNE_IMM, 1, 1 },
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
    } else if (type_is_optional(ts)) {
        rules = optional_rules;
        num_rules = COUNT_OF(optional_rules);
    } else if (type_is_bool(ts)) {
        rules = bool_rules;
        num_rules = COUNT_OF(bool_rules);
    } else {
        UNREACHABLE();
    }

    for (int i = 0; i < num_rules; i++) {
        if (rules[i].ir_op == ir_op) {
            return (BinaryRule *)&rules[i];
        }
    }

    return NULL;
}

typedef struct _LowerConstRule {
    OpCode imm_op;
    OpCode tag_op;
    OpCode load_op;
    OpCode uimm_op;
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
            if (R->imm_op == OP_LOAD_INT_IMM) {
                if (imm >= INT12_MIN && imm <= INT12_MAX) {
                    op = R->imm_op;
                } else {
                    op = R->load_op;
                }
            } else {
                if (imm >= INT16_MIN && imm <= INT16_MAX) {
                    op = R->imm_op;
                } else {
                    op = R->load_op;
                }
            }
            break;
        }

        case CONST_UINT: {
            uint64_t uimm = (uint64_t)c->ival;
            if (R->imm_op == OP_LOAD_INT_IMM) {
                if (uimm <= UINT12_MAX) {
                    op = R->uimm_op;
                } else {
                    op = R->load_op;
                }
            } else {
                if (uimm <= UINT16_MAX) {
                    op = R->uimm_op;
                } else {
                    op = R->load_op;
                }
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

        case CONST_STR:
        case CONST_LIST:
        case CONST_TUPLE: {
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

    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK, OP_LOAD_UINT_IMM };
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
            if (rc->which == CONST_BOOL) {
                // special case for bool: always use imm form, no need to materialize
                insn->code = R->imm_op;
                return;
            }

            if (rc->which == CONST_NONE) {
                // special case for none: always use imm form, no need to materialize
                insn->code = R->imm_op;
                return;
            }

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

        LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK, OP_LOAD_UINT_IMM };
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
    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK, OP_LOAD_UINT_IMM };
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
    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK, OP_LOAD_UINT_IMM };
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
    LowerConstRule R = { OP_RET_INT_IMM, OP_RET_TAG, OP_RET_CONST, OP_RET_UINT_IMM };
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
    LowerConstRule R = { OP_LOAD_INT_IMM, OP_LOAD_TAG, OP_LOADK, OP_LOAD_UINT_IMM };
    OpCode op = get_const_op(c, &R);
    insn->code = op;
}

static int int_need_cast(TypeSpec *dst, TypeSpec *src)
{
    int src_sign = src->int_flt_info.sign;
    int dst_sign = dst->int_flt_info.sign;
    int src_width = src->int_flt_info.width;
    int dst_width = dst->int_flt_info.width;

    ASSERT(dst->kind == TYPE_INT && src->kind == TYPE_INT);

    log_info("[isel] src=%s%d → dst=%s%d", src_sign ? "int" : "uint", src_width * 8,
             dst_sign ? "int" : "uint", dst_width * 8);

    if (dst_sign == src_sign && dst_width >= src_width) {
        log_info("[isel] int, no cast needed: same sign and dst is wider or equal");
        return 0;
    }

    if (dst_sign == 1 && src_sign == 0 && dst_width > src_width) {
        log_info(
            "[isel] int, no cast needed: src is unsigned, dst is signed and wider "
            "(zero-extended)");
        return 0;
    }

    log_info("[isel] int, cast needed");
    return 1;
}

static int encode_int_cast_flag(TypeSpec *dst, TypeSpec *src)
{
    int flag = 0;
    int mode = int_cast_mode();

    ASSERT(dst->kind == TYPE_INT && src->kind == TYPE_INT);

    // 1,2,4,8 → 0,1,2,3
    int dst_width = __builtin_ctz(dst->int_flt_info.width);

    // 0=i, 1=u
    int dst_sign = dst->int_flt_info.sign;
    dst_sign = dst_sign ? 0 : 1;

    int ti = 0b1000 + (dst_sign << 2) + dst_width;
    flag |= ti << 2;
    flag |= mode;

    log_info("[isel] encode_int_cast_flag: 0x%x (src=%s%d → dst=%s%d)", flag,
             src->int_flt_info.sign ? "int" : "uint", src->int_flt_info.width * 8,
             dst_sign ? "int" : "uint", dst_width * 8);

    return flag;
}

static int float_need_cast(TypeSpec *dst, TypeSpec *src)
{
    int src_width = src->int_flt_info.width;
    int dst_width = dst->int_flt_info.width;

    ASSERT(dst->kind == TYPE_FLOAT && src->kind == TYPE_FLOAT);

    log_info("[isel] src=float%d → dst=float%d", src_width * 8, dst_width * 8);

    if (dst_width == src_width) {
        log_info("[isel] float, no cast needed: same type");
        return 0;
    }

    log_info("[isel] float, cast needed: type differs");
    return 1;
}

static int encode_float_cast_flag(TypeSpec *dst, TypeSpec *src)
{
    int flag = 0;
    int mode = float_cast_mode();

    ASSERT(dst->kind == TYPE_FLOAT && src->kind == TYPE_FLOAT);

    // 2,4,8 → 1,2,3
    int dst_width = __builtin_ctz(dst->int_flt_info.width);

    int ti = 0b010000 + dst_width;
    flag |= ti << 2;
    flag |= mode;

    log_info("[isel] encode_float_cast_flag: 0x%x (src=float%d → dst=float%d)", flag,
             src->int_flt_info.width * 8, dst_width * 8);

    return flag;
}

static void isel_lower_cast(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *src = insn_oper_value(insn, 0);
    ASSERT(klr_is_insn(src) || klr_is_param(src) || klr_is_const(src));
    TypeSpec *dst_ts = insn->ts;
    TypeSpec *src_ts = src->ts;

    if (dst_ts->kind == TYPE_INT && src_ts->kind == TYPE_INT) {
        if (int_need_cast(dst_ts, src_ts)) {
            insn->code = OP_INT_CAST;
            insn->cast_flag = encode_int_cast_flag(dst_ts, src_ts);
        }
    } else if (dst_ts->kind == TYPE_FLOAT && src_ts->kind == TYPE_FLOAT) {
        if (float_need_cast(dst_ts, src_ts)) {
            insn->code = OP_FLOAT_CAST;
            insn->cast_flag = encode_float_cast_flag(dst_ts, src_ts);
        }
    } else if (type_is_optional(src_ts) && !type_is_optional(dst_ts)) {
        // opt-ref to non-opt-ref cast, do nothing
    } else {
        NYI();
    }
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

static inline int is_float_cmp(OpCode op)
{
    return (op >= OP_FLOAT_CMPEQ) && (op <= OP_FLOAT_CMPGE);
}

static OpCode float_cmp_map[] = {
    OP_JMP_FLOAT_EQ, OP_JMP_FLOAT_NE, OP_JMP_FLOAT_LT,
    OP_JMP_FLOAT_LE, OP_JMP_FLOAT_GT, OP_JMP_FLOAT_GE,
};

static inline int is_ref_cmp(OpCode op) { return (op >= OP_REF_EQ) && (op <= OP_REF_NE_NULL); }

static OpCode ref_cmp_map[] = {
    OP_JMP_REF_EQ,
    OP_JMP_REF_NE,
    OP_JMP_REF_EQ_NULL,
    OP_JMP_REF_NE_NULL,
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

    if (is_float_cmp(prev->code)) {
        KlrValue *lhs = insn_oper_value(prev, 0);
        KlrValue *rhs = insn_oper_value(prev, 1);
        int idx = prev->code - OP_FLOAT_CMPEQ;
        ASSERT(idx >= 0 && idx < COUNT_OF(float_cmp_map));
        insn->code = float_cmp_map[idx];
        set_operand_at(insn, 0, lhs);
        set_operand_at(insn, 1, rhs);
        klr_erase_insn(prev);
        return;
    }

    if (is_ref_cmp(prev->code)) {
        KlrValue *lhs = insn_oper_value(prev, 0);
        KlrValue *rhs = insn_oper_value(prev, 1);
        int idx = prev->code - OP_REF_EQ;
        ASSERT(idx >= 0 && idx < COUNT_OF(ref_cmp_map));
        insn->code = ref_cmp_map[idx];
        set_operand_at(insn, 0, lhs);
        set_operand_at(insn, 1, rhs);
        klr_erase_insn(prev);
        return;
    }

    if (prev->code == OP_LNOT) {
        KlrValue *val = insn_oper_value(prev, 0);
        KlrValue *true_bb = insn_oper_value(insn, 2);
        KlrValue *false_bb = insn_oper_value(insn, 3);
        set_operand_at(insn, 0, val);
        set_operand_at(insn, 2, false_bb);
        set_operand_at(insn, 3, true_bb);
        // fuse lnot + jmp_cond into a single jmp_cond with inverted condition
        klr_erase_insn(prev);
        return;
    }
}

static void isel_lower_new(KlrInsn *insn, KlrFunc *fn)
{
    int nargs = insn->num_opers;
    for (int i = 1; i < nargs; i++) {
        KlrValue *arg = insn_oper_value(insn, i);
        lower_call_argument(insn, arg, i - 1);
    }

    for (int i = 1; i < nargs; i++) {
        clear_operand_at(insn, i);
    }

    insn->code = OP_NEW;
    insn->num_opers = 1;
    insn->num_args = nargs - 1;
    ASSERT(insn->num_args >= 0);
}

static void isel_lower_build_intern(KlrInsn *insn, KlrFunc *fn)
{
    int nargs = insn->num_opers;
    for (int i = 0; i < nargs; i++) {
        KlrValue *arg = insn_oper_value(insn, i);
        lower_call_argument(insn, arg, i);
    }

    for (int i = 0; i < nargs; i++) {
        clear_operand_at(insn, i);
    }

    insn->num_opers = 0;
    insn->num_args = nargs;
    ASSERT(insn->num_args >= 0);
}

static void verify_insn(KlrInsn *insn)
{
    OpCode op = insn->code;

    if ((op >= OP_BINARY_ADD && op <= OP_IR_PHI) || (op == OP_JMP) || (op == OP_RET) ||
        (op == OP_RET_VOID) || (op == OP_MOVE) || (op == OP_GLOBAL_GET) || (op == OP_GLOBAL_SET) ||
        (op == OP_LAND) || (op == OP_LOR) || (op == OP_LNOT) || (op == OP_IR_NEW) ||
        (op == OP_BUILD_INTERN)) {
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

                case OP_IR_CAST: {
                    isel_lower_cast(insn, fn);
                    break;
                }

                case OP_IR_JMP_COND: {
                    if (fusion_enabled()) {
                        isel_lower_jmp_cond(insn, fn);
                    }
                    break;
                }

                case OP_BUILD_INTERN: {
                    isel_lower_build_intern(insn, fn);
                    break;
                }

                case OP_IR_NEW: {
                    isel_lower_new(insn, fn);
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
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
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
