/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "cmd.h"
#include "ir.h"
#include "log.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

int type_allowed_to_prop(TypeSpec *ts)
{
    // only propagate simple types, don't propagate tuple/list/dict/class(not constant unique)
    if (ts->kind == TYPE_INT || ts->kind == TYPE_FLOAT || ts->kind == TYPE_BOOL ||
        ts->kind == TYPE_STR || type_is_range(ts) || type_is_tuple(ts)) {
        return 1;
    }

    if (type_is_optional(ts)) {
        TypeSpec *src = ts->opt.src;
        if (!src) return 1; // None is allowed to propagate
        return type_allowed_to_prop(src);
    }

    return 0;
}

static int check_int_const_cast_valid(KlrInsn *insn, KlrConst *c, KlrModule *m)
{
    TypeSpec *dst_ts = insn->ts;
    if (dst_ts->kind != TYPE_INT) return 0;

    int dst_width = dst_ts->int_flt_info.width;
    if (c->which == CONST_INT) {
        int src_width = c->len;
        if (src_width > dst_width) {
            int64_t val = (int64_t)c->ival;
            if (dst_width == 1) {
                if (val < INT8_MIN || val > INT8_MAX) {
                    KlrLocInfo *loc = &insn->loc;
                    klr_error(loc,
                              "constant integer overflow in cast: cannot cast from int%d to int%d",
                              src_width * 8, dst_width * 8);
                    insn->error = 1;
                    return -1;
                }
            } else if (dst_width == 2) {
                if (val < INT16_MIN || val > INT16_MAX) {
                    KlrLocInfo *loc = &insn->loc;
                    klr_error(loc,
                              "constant integer overflow in cast: cannot cast from int%d to int%d",
                              src_width * 8, dst_width * 8);
                    insn->error = 1;
                    return -1;
                }
            } else if (dst_width == 4) {
                if (val < INT32_MIN || val > INT32_MAX) {
                    KlrLocInfo *loc = &insn->loc;
                    klr_error(loc,
                              "constant integer overflow in cast: cannot cast from int%d to int%d",
                              src_width * 8, dst_width * 8);
                    insn->error = 1;
                    return -1;
                }
            }
        }
    } else if (c->which == CONST_UINT) {
        int src_width = c->len;
        if (src_width > dst_width) {
            if (dst_width == 1) {
                if (c->ival > UINT8_MAX) {
                    KlrLocInfo *loc = &insn->loc;
                    klr_error(loc,
                              "constant integer overflow in cast: cannot cast from "
                              "uint%d to int%d",
                              src_width * 8, dst_width * 8);
                    insn->error = 1;
                    return -1;
                }
            } else if (dst_width == 2) {
                if (c->ival > UINT16_MAX) {
                    KlrLocInfo *loc = &insn->loc;
                    klr_error(loc,
                              "constant integer overflow in cast: cannot cast from "
                              "uint%d to int%d",
                              src_width * 8, dst_width * 8);
                    insn->error = 1;
                    return -1;
                }
            } else if (dst_width == 4) {
                if (c->ival > UINT32_MAX) {
                    KlrLocInfo *loc = &insn->loc;
                    klr_error(loc,
                              "constant integer overflow in cast: cannot cast from "
                              "uint%d to int%d",
                              src_width * 8, dst_width * 8);
                    insn->error = 1;
                    return -1;
                }
            }
        }
    }
    return 0;
}

static int check_float_const_cast_valid(KlrInsn *insn, KlrConst *c, KlrModule *m, double *out)
{
    TypeSpec *dst_ts = insn->ts;
    if (dst_ts->kind != TYPE_FLOAT) return 0;

    int mode = float_cast_mode();

    int dst_width = dst_ts->int_flt_info.width;
    int src_width = c->len;
    if (src_width > dst_width) {
        double v = c->fval;
        if (dst_width == 2) {
            _Float16 h = (_Float16)v;
            double rt = (double)h;
            if (mode == 0 && rt != v) {
                KlrLocInfo *loc = &insn->loc;
                klr_error(loc,
                          "constant float overflow in cast: cannot cast from float%d to float%d",
                          src_width * 8, dst_width * 8);
                insn->error = 1;
                return -1;
            }
            *out = rt;
        } else if (dst_width == 4) {
            float f = (float)v;
            double rt = (double)f;
            if (mode == 0 && rt != v) {
                KlrLocInfo *loc = &insn->loc;
                klr_error(loc,
                          "constant float overflow in cast: cannot cast from float%d to float%d",
                          src_width * 8, dst_width * 8);
                insn->error = 1;
                return -1;
            }
            *out = rt;
        }
    }
    return 0;
}

#define int64_add_overflow __builtin_add_overflow
#define int64_sub_overflow __builtin_sub_overflow
#define int64_mul_overflow __builtin_mul_overflow

#define uint64_add_overflow __builtin_add_overflow
#define uint64_sub_overflow __builtin_sub_overflow
#define uint64_mul_overflow __builtin_mul_overflow

static int int64_div_overflow(int64_t a, int64_t b, int64_t *out)
{
    if (b == 0) return -1; // division by zero

    if (a == INT64_MIN && b == -1) return -1; // only overflow case

    *out = a / b;
    return 0;
}

static int uint64_div_overflow(uint64_t a, uint64_t b, uint64_t *out)
{
    if (b == 0) return -1;

    *out = a / b;
    return 0;
}

static int int64_mod_overflow(int64_t a, int64_t b, int64_t *out)
{
    if (b == 0) return -1; // division by zero

    if (a == INT64_MIN && b == -1) return -1; // only overflow case

    *out = a % b;
    return 0;
}

static int uint64_mod_overflow(uint64_t a, uint64_t b, uint64_t *out)
{
    if (b == 0) return -1;

    *out = a % b;
    return 0;
}

// clang-format off

#define DEFINE_FOLD_BINARY_FUNC(opname, op) \
    static void __fold_binary_##opname##_insn(KlrInsn *insn, KlrModule *m) \
    { \
        KlrValue *lhs = insn_oper_value(insn, 0); \
        KlrValue *rhs = insn_oper_value(insn, 1); \
        if (klr_is_const(lhs) && klr_is_const(rhs)) { \
            KlrConst *lval = (KlrConst *)lhs; \
            KlrConst *rval = (KlrConst *)rhs; \
            if (lval->which == CONST_INT && rval->which == CONST_INT) { \
                log_info("fold binary" #opname "insn to const int:"); \
                log_insn(insn); \
                int64_t res; \
                int r = int64_##opname##_overflow((int64_t)lval->ival, \
                                                   (int64_t)rval->ival, &res); \
                if (r) { \
                    KlrLocInfo *loc = &insn->loc; \
                    klr_error(loc, "signed integer overflow in binary" #opname); \
                    insn->error = 1; \
                    return; \
                } \
                KlrValue *const_res = klr_const_int(res, lval->ts, m); \
                replace_all_uses_with(const_res, (KlrValue *)insn); \
            } else if (lval->which == CONST_UINT && rval->which == CONST_UINT) { \
                log_info("fold binary" #opname "insn to const uint:"); \
                log_insn(insn); \
                uint64_t res; \
                int r = uint64_##opname##_overflow(lval->ival, rval->ival, &res); \
                if (r) { \
                    KlrLocInfo *loc = &insn->loc; \
                    klr_error(loc, "unsigned integer overflow in binary" #opname); \
                    insn->error = 1; \
                    return; \
                } \
                KlrValue *const_res = klr_const_uint(res, lval->ts, m); \
                replace_all_uses_with(const_res, (KlrValue *)insn); \
            } else if (lval->which == CONST_FLT && rval->which == CONST_FLT) { \
                log_info("fold binary" #opname "insn to const float:"); \
                log_insn(insn); \
                double res = lval->fval op rval->fval; \
                KlrValue *const_res = klr_const_float(res, lval->ts, m); \
                replace_all_uses_with(const_res, (KlrValue *)insn); \
            } \
        } \
    }

#define DEFINE_FOLD_BINARY_FUNC_NO_OVERFLOW(opname, op) \
    static void __fold_binary_##opname##_insn(KlrInsn *insn, KlrModule *m) \
    { \
        KlrValue *lhs = insn_oper_value(insn, 0); \
        KlrValue *rhs = insn_oper_value(insn, 1); \
        if (klr_is_const(lhs) && klr_is_const(rhs)) { \
            KlrConst *lval = (KlrConst *)lhs; \
            KlrConst *rval = (KlrConst *)rhs; \
            if (lval->which == CONST_INT && rval->which == CONST_INT) { \
                log_info("fold binary" #opname "insn to const int:"); \
                log_insn(insn); \
                int64_t res = (int64_t)lval->ival op (int64_t)rval->ival; \
                KlrValue *const_res = klr_const_int(res, lval->ts, m); \
                replace_all_uses_with(const_res, (KlrValue *)insn); \
            } else if (lval->which == CONST_UINT && rval->which == CONST_UINT) { \
                log_info("fold binary" #opname "insn to const uint:"); \
                log_insn(insn); \
                uint64_t res = lval->ival op rval->ival; \
                KlrValue *const_res = klr_const_uint(res, lval->ts, m); \
                replace_all_uses_with(const_res, (KlrValue *)insn); \
            } \
        } \
    }

// clang-format on

DEFINE_FOLD_BINARY_FUNC(add, +)
DEFINE_FOLD_BINARY_FUNC(sub, -)
DEFINE_FOLD_BINARY_FUNC(mul, *)
DEFINE_FOLD_BINARY_FUNC(div, /)

static void __fold_binary_mod_insn(KlrInsn *insn, KlrModule *m)
{
    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);
    if (klr_is_const(lhs) && klr_is_const(rhs)) {
        KlrConst *lval = (KlrConst *)lhs;
        KlrConst *rval = (KlrConst *)rhs;
        if (lval->which == CONST_INT && rval->which == CONST_INT) {
            log_info("fold binary mod insn to const int:");
            log_insn(insn);
            int64_t res;
            int r = int64_mod_overflow((int64_t)lval->ival, (int64_t)rval->ival, &res);
            if (r) {
                KlrLocInfo *loc = &insn->loc;
                klr_error(loc, "signed integer overflow in binary mod");
                insn->error = 1;
                return;
            }
            KlrValue *const_res = klr_const_int(res, lval->ts, m);
            replace_all_uses_with(const_res, (KlrValue *)insn);
        } else if (lval->which == CONST_UINT && rval->which == CONST_UINT) {
            log_info("fold binary mod insn to const uint:");
            log_insn(insn);
            uint64_t res;
            int r = uint64_mod_overflow(lval->ival, rval->ival, &res);
            if (r) {
                KlrLocInfo *loc = &insn->loc;
                klr_error(loc, "unsigned integer overflow in binary mod");
                insn->error = 1;
                return;
            }
            KlrValue *const_res = klr_const_uint(res, lval->ts, m);
            replace_all_uses_with(const_res, (KlrValue *)insn);
        } else if (lval->which == CONST_FLT && rval->which == CONST_FLT) {
            log_info("fold binary mod insn to const float:");
            log_insn(insn);
            double res = fmod(lval->fval, rval->fval);
            if (res < 0) res += rval->fval;
            KlrValue *const_res = klr_const_float(res, lval->ts, m);
            replace_all_uses_with(const_res, (KlrValue *)insn);
        }
    }
}

DEFINE_FOLD_BINARY_FUNC_NO_OVERFLOW(and, &)
DEFINE_FOLD_BINARY_FUNC_NO_OVERFLOW(or, |)
DEFINE_FOLD_BINARY_FUNC_NO_OVERFLOW(xor, ^)

static int do_fold(KlrInsn *insn, KlrFunc *fn)
{
    KlrModule *m = fn->module;
    OpCode op = insn->code;

    if (op == OP_GLOBAL_SET || op == OP_GLOBAL_GET) {
        return 0;
    }

    if (op == OP_IR_CALL && (insn->flags & KLR_INSN_FLAGS_CONST)) {
        return 0;
    }

    int changed = 0;

    KlrUse *use;
    insn_oper_use_foreach(use, insn) {
        if (use->is_def) continue;
        KlrValue *val = use->ref;
        if (val && klr_is_local(val)) {
            KlrInsn *src = (KlrInsn *)val;
            // from current basic block local variable map, get the latest
            // value(const/insn) for this local variable
            KlrValue *_val = klr_get_local_var(insn->bb, src);
            if (_val) {
                if (klr_is_const(_val)) {
                    // Constant Propagation: Always replace local variables with
                    // known constants to enable further folding.
                    log_info("operand %d-th of insn(/) is const value:", i__);
                    log_insn(insn);
                    set_operand_at(insn, i__, _val);
                    changed = 1;
                } else {
                    if (insn->code == OP_RET) {
                        // Special Case for Return: Enable operand forwarding even for
                        // non-constants. This bypasses the local variable, making its
                        // last 'move' instruction redundant and eligible for DCE.
                        log_info("ret operand optimized: forwarding non-const value");
                        set_operand_at(insn, i__, _val);
                        changed = 1;
                    } else {
                        // Non-Constant Propagation: We generally disallow this to
                        // avoid extending variable lifetimes, which would complicate
                        // register allocation in our non-pure SSA IR. (Ref:
                        // bench/loop-sum.kl)
                        log_info(
                            "operand %d-th of insn(/) cannot be replaced with non-const "
                            "value/insn:",
                            i__);
                        log_insn(insn);
                        klr_clear_local_var(insn->bb, src);
                    }
                }
            }
        }
    }

    switch (op) {
        case OP_BINARY_ADD: {
            __fold_binary_add_insn(insn, m);
            break;
        }

        case OP_BINARY_SUB: {
            __fold_binary_sub_insn(insn, m);
            break;
        }

        case OP_BINARY_MUL: {
            __fold_binary_mul_insn(insn, m);
            break;
        }

        case OP_BINARY_DIV: {
            __fold_binary_div_insn(insn, m);
            break;
        }

        case OP_BINARY_MOD: {
            __fold_binary_mod_insn(insn, m);
            break;
        }

        case OP_BINARY_AND: {
            __fold_binary_and_insn(insn, m);
            break;
        }
        case OP_BINARY_OR: {
            __fold_binary_or_insn(insn, m);
            break;
        }
        case OP_BINARY_XOR: {
            __fold_binary_xor_insn(insn, m);
            break;
        }

        case OP_BINARY_CMPGT: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                KlrConst *rval = (KlrConst *)rhs;
                if ((lval->which == CONST_INT && rval->which == CONST_INT)) {
                    log_info("fold binary cmp_gt(int) insn to const bool:");
                    log_insn(insn);
                    int res = (int64_t)lval->ival > (int64_t)rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                } else if ((lval->which == CONST_UINT && rval->which == CONST_UINT)) {
                    log_info("fold binary cmp_gt(uint) insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival > rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMPGE: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                KlrConst *rval = (KlrConst *)rhs;
                if ((lval->which == CONST_INT && rval->which == CONST_INT)) {
                    log_info("fold binary cmp_ge(int) insn to const bool:");
                    log_insn(insn);
                    int res = (int64_t)lval->ival >= (int64_t)rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                } else if ((lval->which == CONST_UINT && rval->which == CONST_UINT)) {
                    log_info("fold binary cmp_ge(uint) insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival >= rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMPLT: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                KlrConst *rval = (KlrConst *)rhs;
                if ((lval->which == CONST_INT && rval->which == CONST_INT)) {
                    log_info("fold binary cmp_lt(int) insn to const bool:");
                    log_insn(insn);
                    int res = (int64_t)lval->ival < (int64_t)rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                } else if ((lval->which == CONST_UINT && rval->which == CONST_UINT)) {
                    log_info("fold binary cmp_lt(uint) insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival < rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMPLE: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                KlrConst *rval = (KlrConst *)rhs;
                if ((lval->which == CONST_INT && rval->which == CONST_INT)) {
                    log_info("fold binary cmp_le(int) insn to const bool:");
                    log_insn(insn);
                    int res = (int64_t)lval->ival <= (int64_t)rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                } else if ((lval->which == CONST_UINT && rval->which == CONST_UINT)) {
                    log_info("fold binary cmp_le(uint) insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival <= rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMPEQ: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                KlrConst *rval = (KlrConst *)rhs;
                if ((lval->which == CONST_INT && rval->which == CONST_INT) ||
                    (lval->which == CONST_UINT && rval->which == CONST_UINT)) {
                    log_info("fold binary cmp_eq insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival == rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMPNE: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                KlrConst *rval = (KlrConst *)rhs;
                if ((lval->which == CONST_INT && rval->which == CONST_INT) ||
                    (lval->which == CONST_UINT && rval->which == CONST_UINT)) {
                    log_info("fold binary cmp_ne insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival != rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->module);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_LAND: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);

            ASSERT(lhs->ts == rhs->ts);
            ASSERT(lhs->ts = bool_type_spec());

            // Short-circuiting
            if (klr_is_const(lhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                ASSERT(lval->which == CONST_BOOL);
                if (!lval->bval) {
                    log_info("[Short-circuiting] fold AND insn, the left is false:");
                    log_insn(insn);
                    // false && x -> false
                    KlrValue *res = klr_const_bool(0, fn->module);
                    replace_all_uses_with(res, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold AND insn, the left is true:");
                    log_insn(insn);
                    // true && x -> x
                    replace_all_uses_with(rhs, (KlrValue *)insn);
                }
            } else if (klr_is_const(rhs)) {
                KlrConst *rval = (KlrConst *)rhs;
                ASSERT(rval->which == CONST_BOOL);
                if (!rval->bval) {
                    log_info(
                        "[Short-circuiting] fold AND insn, the right is "
                        "false:");
                    log_insn(insn);
                    // x && false -> false
                    KlrValue *res = klr_const_bool(0, fn->module);
                    replace_all_uses_with(res, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold AND insn, the right is true:");
                    log_insn(insn);
                    // x && true -> x
                    replace_all_uses_with(lhs, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_LOR: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);

            ASSERT(lhs->ts == rhs->ts);
            ASSERT(lhs->ts == bool_type_spec());

            // Short-circuiting
            if (klr_is_const(lhs)) {
                KlrConst *lval = (KlrConst *)lhs;
                ASSERT(lval->which == CONST_BOOL);
                if (lval->bval) {
                    log_info("[Short-circuiting] fold OR insn, the left is true:");
                    log_insn(insn);
                    // true || x -> true
                    KlrValue *res = klr_const_bool(1, fn->module);
                    replace_all_uses_with(res, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold OR insn, the left is false:");
                    log_insn(insn);
                    // false || x -> x
                    replace_all_uses_with(rhs, (KlrValue *)insn);
                }
            } else if (klr_is_const(rhs)) {
                KlrConst *rval = (KlrConst *)rhs;
                ASSERT(rval->which == CONST_BOOL);
                if (rval->bval) {
                    log_info("[Short-circuiting] fold OR insn, the right is true:");
                    log_insn(insn);
                    // x || true -> true
                    KlrValue *res = klr_const_bool(1, fn->module);
                    replace_all_uses_with(res, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold OR insn, the right is false:");
                    log_insn(insn);
                    // x || false -> x
                    replace_all_uses_with(lhs, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_IR_SELECT: {
            KlrValue *cond = insn_oper_value(insn, 0);
            KlrValue *true_val = insn_oper_value(insn, 1);
            KlrValue *false_val = insn_oper_value(insn, 2);
            if (klr_is_const(cond)) {
                KlrConst *c = (KlrConst *)cond;
                ASSERT(c->which == CONST_BOOL);
                if (c->bval) {
                    log_info("[Short-circuiting] fold SELECT insn, condition is true:");
                    log_insn(insn);
                    replace_all_uses_with(true_val, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold SELECT insn, condition is false:");
                    log_insn(insn);
                    replace_all_uses_with(false_val, (KlrValue *)insn);
                }
            }
            break;
        }

        default: {
            break;
        }
    }

    return changed;
}

static int do_propagate(KlrInsn *insn, KlrFunc *fn)
{
    KlrModule *m = fn->module;
    int changed = 0;
    OpCode op = insn->code;
    switch (op) {
        case OP_GLOBAL_SET: {
            KlrGlobal *global = (KlrGlobal *)insn_oper_value(insn, 0);
            KlrValue *val = insn_oper_value(insn, 1);
            if (!global->mutable && klr_is_const(val)) {
                log_info("record const value for global variable:");
                log_insn(insn);
                global->kval = (KlrConst *)val;
            }
            break;
        }

        case OP_GLOBAL_GET: {
            KlrGlobal *global = (KlrGlobal *)insn_oper_value(insn, 0);
            KlrConst *val = global->kval;
            // var a = 100
            // let b = a
            // Because `a` is mutable, we cannot propagate `a` to `b`
            // When `let c = b + 1`, `get_global 'b' is const, but val is null.
            if (!global->mutable && val) {
                log_info("propagate const global variable:");
                log_insn(insn);
                replace_all_uses_with((KlrValue *)val, (KlrValue *)insn);
            }
            break;
        }

        case OP_MOVE: {
            // move is one of which doesn't have uses.
            /*
            These instructions, which include move, jmp, branch, return,
            set_global, and call_void, produce no SSA result value, so there are
            no values that could be used by other instructions and their
            use‑lists are empty.
            */
            ASSERT(!klr_is_used(insn));
            KlrValue *_dst = insn_oper_value(insn, 0);
            KlrValue *src = insn_oper_value(insn, 1);
            ASSERT(klr_is_local(_dst));
            KlrInsn *dst = (KlrInsn *)_dst;

            if (dst->flags & KLR_INSN_FLAGS_CONST) {
                // dst is let variable.
                if (type_allowed_to_prop(src->ts)) {
                    log_info("propagate const local variable:");
                    log_insn(insn);
                    // dst is let: global propagation, no SSA needed
                    // if src is const, this is const propagation, otherwise this is
                    // copy propagation -> let x = y; let z = x -> let z = y This
                    // handles both Constant Prop (x = 10) and Copy Prop (x = %0).
                    replace_all_uses_with(src, _dst);
                } else if (klr_is_insn(src)) {
                    KlrInsn *src_insn = (KlrInsn *)src;
                    if ((src_insn->code == OP_MAKE_INTF || src_insn->code == OP_UPCAST_INTF) &&
                        src->use_count == 1) {
                        // Special Case for Interface Creation: If the source is an OP_MAKE_INTF
                        // or OP_UPCAST_INTF instruction with only one use, we can safely propagate
                        // it even if it's not a constant. This is because these instructions
                        // typically create a new interface value that is immutable after creation,
                        // and propagating it can enable further optimizations without risking
                        // unintended side effects.
                        log_info(
                            "propagate non-const OP_MAKE_INTF or OP_UPCAST_INTF to let variable:");
                        log_insn(insn);
                        replace_all_uses_with(src, _dst);
                    }
                }
            } else {
                // dst is var: local propagation, only one basic block, no SSA
                // needed
                if (klr_is_const(src)) {
                    log_info("update var local's const value in bb '%s'",
                             klr_block_name(insn->bb));
                    log_insn(insn);
                    /* Record the latest constant value in the local BB map */
                    KlrBasicBlock *bb = insn->bb;
                    klr_update_local_var(bb, dst, src, insn);
                } else {
                    log_info(
                        "update var local's value in bb '%s' although it's "
                        "assigned a volatile value",
                        klr_block_name(insn->bb));
                    // Variable is assigned a volatile value, also update it
                    // This is var copy propagation, if src is not const, we can
                    // still propagate src to dst.
                    KlrBasicBlock *bb = insn->bb;
                    if (src->kind == KLR_VALUE_PARAM) {
                        // parameter is immutable, we can propagate it in func
                        // scope
                        klr_update_local_var(bb, dst, src, insn);
                    } else if (src->kind == KLR_VALUE_INSN) {
                        KlrInsn *_insn = (KlrInsn *)src;
                        if (_insn->flags & KLR_INSN_FLAGS_CONST) {
                            // src is const insn, we can propagate it in func
                            // scope
                            klr_update_local_var(bb, dst, src, insn);
                        } else {
                            if (_insn->bb == bb) {
                                // Only propagate if src is defined in the same
                                // basic block, otherwise it's not safe to
                                // propagate.
                                klr_update_local_var(bb, dst, src, insn);
                            } else {
                                klr_clear_local_var(bb, dst);
                            }
                        }
                    } else {
                        UNREACHABLE();
                    }
                }
            }
            break;
        }

        case OP_IR_CALL: {
            if (insn->flags & KLR_INSN_FLAGS_CONST) {
                KlrValue *callee = insn_oper_value(insn, 0);

                if (callee->kind != KLR_VALUE_KLASS) {
                    log_info("unsupported const call to non-class.");
                    break;
                }

                KlrValue *items[insn->num_opers - 1];
                memset(items, 0, sizeof(items));
                // skip callee operand
                KlrValue *val;
                _insn_oper_value_foreach(val, insn, 1) {
                    ASSERT(klr_is_const(val));
                    items[i__ - 1] = val;
                }

                if (!strcmp(callee->name, "list")) {
                    val = klr_const_list(items, insn->num_opers - 1, insn->ts, fn->module);
                } else if (!strcmp(callee->name, "tuple")) {
                    val = klr_const_tuple(items, insn->num_opers - 1, insn->ts, fn->module);
                } else if (!strcmp(callee->name, "int64")) {
                    ASSERT(insn->num_opers == 2);
                    val = insn_oper_value(insn, 1);
                    ASSERT(klr_is_const(val));
                } else {
                    printf("unsupported const call to class '%s'\n", callee->name);
                    NYI();
                }

                replace_all_uses_with(val, (KlrValue *)insn);
            } else {
                log_info("no propagation for non-const call insn:");
                log_insn(insn);
            }
            break;
        }

        case OP_IR_CAST: {
            KlrValue *src = insn_oper_value(insn, 0);
            if ((insn->error == 0) && klr_is_const(src)) {
                KlrConst *c = (KlrConst *)src;
                if (c->which == CONST_INT || c->which == CONST_UINT) {
                    log_info("fold const cast insn:");
                    log_insn(insn);
                    if (!check_int_const_cast_valid(insn, c, m)) {
                        int sign = insn->ts->int_flt_info.sign;
                        KlrValue *v;
                        if (sign) {
                            v = klr_const_int(c->ival, insn->ts, fn->module);
                        } else {
                            v = klr_const_uint(c->ival, insn->ts, fn->module);
                        }
                        if (replace_all_uses_with(v, (KlrValue *)insn)) {
                            changed = 1;
                        }
                    }
                } else if (c->which == CONST_FLT) {
                    log_info("fold const cast insn:");
                    log_insn(insn);
                    double f = 0.0;
                    if (!check_float_const_cast_valid(insn, c, m, &f)) {
                        KlrValue *v = klr_const_float(f, insn->ts, fn->module);
                        if (replace_all_uses_with(v, (KlrValue *)insn)) {
                            changed = 1;
                        }
                    }
                } else {
                    log_info("unsupported const cast for non-int/uint/float constant:");
                    log_insn(insn);
                }
            }
            break;
        }

        default: {
            break;
        }
    }

    return changed;
}

static int do_const_range_fold(KlrInsn *insn, KlrFunc *fn)
{
    if (insn->flags & KLR_INSN_FLAGS_DEAD) {
        return 0;
    }

    OpCode op = insn->code;
    if (op != OP_GET_FIELD_EXT) {
        return 0;
    }

    KlrValue *obj = insn_oper_value(insn, 0);
    if (!klr_is_const(obj)) {
        return 0;
    }

    KlrConst *kc = (KlrConst *)obj;
    if (kc->which != CONST_RANGE) {
        return 0;
    }

    Vector *list = kc->list;

    switch (op) {
        case OP_GET_FIELD_EXT: {
            KlrValue *fld = insn_oper_value(insn, 1);
            if (str_equal(fld->name, "start")) {
                KlrValue *start = vector_at(list, 0);
                replace_all_uses_with(start, (KlrValue *)insn);
                log_info("fold range start field access to const value.");
            } else if (str_equal(fld->name, "stop")) {
                KlrValue *stop = vector_at(list, 1);
                replace_all_uses_with(stop, (KlrValue *)insn);
                log_info("fold range stop field access to const value.");
            } else if (str_equal(fld->name, "step")) {
                KlrValue *step = vector_at(list, 2);
                replace_all_uses_with(step, (KlrValue *)insn);
                log_info("fold range step field access to const value.");
            } else {
                UNREACHABLE();
            }
            insn->flags |= KLR_INSN_FLAGS_DEAD;
            return 0;
        }

        default: {
            UNREACHABLE();
            return 0;
        }
    }
}

/*
The `let` variable is immutable, so it can be propagated.
This is a global constant propagation and no need SSA format.
It can be used to fold list/tuple/map/set literals, and also can be used to fold
const variables.

The `var` variable is mutable, so it can be propagated only one basic block
inside, and only for literal values. This is a local constant propagation and no
need SSA format. It can be used to fold list/tuple/map/set literals, and also
can be used to fold const variables. In one basic block, if there are many store
insns to the same variable, only the last store insn can be propagated, and the
previous store insns will be removed.
*/
int klr_const_copy_prop_pass(KlrFunc *fn, void *data)
{
    KlrModule *m = fn->module;
    int changed = 1;

    while (changed) {
        changed = 0;

        KlrBasicBlock *bb;
        basic_block_foreach(bb, fn) {
            klr_clear_local_var_map(bb);
            KlrInsn *insn;
            insn_foreach(insn, bb) {
                changed |= do_propagate(insn, fn);
                changed |= do_fold(insn, fn);
                changed |= do_const_range_fold(insn, fn);
            }
        }
    }

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn, *next;
        insn_foreach_safe(insn, next, bb) {
            if (insn_is_dead(insn)) {
                ASSERT(insn->code == OP_MOVE || insn->code == OP_GET_FIELD_EXT);
                klr_erase_insn(insn);
            }
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
