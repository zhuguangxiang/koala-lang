/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

static void do_fold(KlrInsn *insn, KlrFunc *fn, Queue *wklist)
{
    OpCode op = insn->code;

    if (op == OP_SET_GLOBAL || op == OP_GET_GLOBAL || op == OP_MOVE) {
        return;
    }

    if (op == OP_CALL && (insn->flags & KLR_INSN_FLAGS_CONST)) {
        return;
    }

    int changed = 0;

    KlrValue *val;
    oper_value_foreach(val, insn, 0) {
        if (klr_is_local(val)) {
            KlrInsn *src = (KlrInsn *)val;
            KlrValue *const_val = klr_get_local_var_const(src->bb, src);
            if (const_val) {
                log_info("operand %d-th of insn(/) is const value", i__);
                log_insn(insn);
                ASSERT(klr_is_const(const_val));
                KlrConst *kval = klr_const_value(const_val);
                update_index_operand(insn, i__, (KlrValue *)kval);
                changed = 1;
            }
        }
    }

    if (changed) {
        queue_push(wklist, insn);
    }

    switch (op) {
        case OP_BINARY_ADD: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary add insn to const int:");
                    log_insn(insn);
                    uint64_t res = lval->ival + rval->ival;
                    KlrValue *const_res = klr_const_int(res, lval->ts, fn->mod);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_SUB: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary sub insn to const int:");
                    log_insn(insn);
                    uint64_t res = lval->ival - rval->ival;
                    KlrValue *const_res = klr_const_int(res, lval->ts, fn->mod);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMP_GT: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary cmp_gt insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival > rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->mod);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMP_GE: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary cmp_ge insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival >= rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->mod);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMP_LT: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary cmp_lt insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival < rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->mod);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMP_LE: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary cmp_le insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival <= rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->mod);
                    replace_all_uses_with(const_res, (KlrValue *)insn);
                }
            }
            break;
        }

        case OP_BINARY_CMP_EQ: {
            KlrValue *lhs = insn_oper_value(insn, 0);
            KlrValue *rhs = insn_oper_value(insn, 1);
            if (klr_is_const(lhs) && klr_is_const(rhs)) {
                KlrConst *lval = klr_const_value(lhs);
                KlrConst *rval = klr_const_value(rhs);
                if (lval->which == CONST_INT && rval->which == CONST_INT) {
                    log_info("fold binary cmp_eq insn to const bool:");
                    log_insn(insn);
                    int res = lval->ival == rval->ival;
                    KlrValue *const_res = klr_const_bool(res, fn->mod);
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
                KlrConst *lval = klr_const_value(lhs);
                ASSERT(lval->which == CONST_BOOL);
                if (!lval->bval) {
                    log_info("[Short-circuiting] fold AND insn, the left is false:");
                    log_insn(insn);
                    // false && x -> false
                    KlrValue *res = klr_const_bool(0, fn->mod);
                    replace_all_uses_with(res, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold AND insn, the left is true:");
                    log_insn(insn);
                    // true && x -> x
                    replace_all_uses_with(rhs, (KlrValue *)insn);
                }
            } else if (klr_is_const(rhs)) {
                KlrConst *rval = klr_const_value(rhs);
                ASSERT(rval->which == CONST_BOOL);
                if (!rval->bval) {
                    log_info("[Short-circuiting] fold AND insn, the right is false:");
                    log_insn(insn);
                    // x && false -> false
                    KlrValue *res = klr_const_bool(0, fn->mod);
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
                KlrConst *lval = klr_const_value(lhs);
                ASSERT(lval->which == CONST_BOOL);
                if (lval->bval) {
                    log_info("[Short-circuiting] fold OR insn, the left is true:");
                    log_insn(insn);
                    // true || x -> true
                    KlrValue *res = klr_const_bool(1, fn->mod);
                    replace_all_uses_with(res, (KlrValue *)insn);
                } else {
                    log_info("[Short-circuiting] fold OR insn, the left is false:");
                    log_insn(insn);
                    // false || x -> x
                    replace_all_uses_with(rhs, (KlrValue *)insn);
                }
            } else if (klr_is_const(rhs)) {
                KlrConst *rval = klr_const_value(rhs);
                ASSERT(rval->which == CONST_BOOL);
                if (rval->bval) {
                    log_info("[Short-circuiting] fold OR insn, the right is true:");
                    log_insn(insn);
                    // x || true -> true
                    KlrValue *res = klr_const_bool(1, fn->mod);
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

        default: {
            break;
        }
    }
}

static void do_propagate(KlrInsn *insn, KlrFunc *fn, Queue *wklist)
{
    OpCode op = insn->code;
    switch (op) {
        case OP_SET_GLOBAL: {
            KlrGlobal *global = (KlrGlobal *)insn_oper_value(insn, 0);
            KlrValue *val = insn_oper_value(insn, 1);
            if (!global->mutable && klr_is_const(val)) {
                log_info("record const value for global variable:");
                log_insn(insn);
                KlrConst *kval = klr_const_value(val);
                ASSERT((void *)kval == (void *)val);
                // update_index_operand(insn, 1, (KlrValue *)kval);
                global->kval = kval;
            }
            break;
        }

        case OP_GET_GLOBAL: {
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
            ASSERT(!klr_value_used(insn));
            KlrValue *_dst = insn_oper_value(insn, 0);
            KlrValue *src = insn_oper_value(insn, 1);
            ASSERT(klr_is_local(_dst));
            KlrInsn *dst = (KlrInsn *)_dst;

            if (dst->flags & KLR_INSN_FLAGS_CONST) {
                log_info("propagate const local variable:");
                log_insn(insn);
                // dst is let: global propagation, no SSA needed
                // if src is const, this is const propagation, otherwise this is copy
                // propagation -> let x = y; let z = x -> let z = y
                // This handles both Constant Prop (x = 10) and Copy Prop (x = %0).
                replace_all_uses_with(src, _dst);
            } else {
                // dst is var: local propagation, only one basic block, no SSA needed
                if (klr_is_const(src)) {
                    log_info("update var local's const value in bb '%s'",
                             klr_block_name(insn->bb));
                    log_insn(insn);
                    /* Record the latest constant alue in the local BB map */
                    KlrBasicBlock *bb = dst->bb;
                    klr_update_local_var_const(bb, dst, klr_const_value(src));
                } else {
                    log_info(
                        "clear var local's const value in bb '%s' because it's assigned "
                        "a volatile value",
                        klr_block_name(insn->bb));
                    // Variable is assigned a volatile value, clear local variable
                    // constant
                    KlrBasicBlock *bb = dst->bb;
                    klr_clear_local_var_const(bb, dst);
                }
            }
            break;
        }

        case OP_CALL: {
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
                oper_value_foreach(val, insn, 1) {
                    ASSERT(klr_is_const(val));
                    KlrConst *kval = klr_const_value(val);
                    items[i__ - 1] = (KlrValue *)kval;
                }

                if (!strcmp(callee->name, "list")) {
                    val = klr_const_list(items, insn->num_opers - 1, insn->ts, fn->mod);
                } else if (!strcmp(callee->name, "tuple")) {
                    val = klr_const_tuple(items, insn->num_opers - 1, insn->ts, fn->mod);
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

        default: {
            break;
        }
    }
}

/*
The `let` variable is immutable, so it can be propagated.
This is a global constant propagation and no need SSA format.
It can be used to fold list/tuple/map/set literals, and also can be used to fold const
variables.

The `var` variable is mutable, so it can be propagated only one basic block inside, and
only for literal values. This is a local constant propagation and no need SSA format. It
can be used to fold list/tuple/map/set literals, and also can be used to fold const
variables. In one basic block, if there are many store insns to the same variable, only
the last store insn can be propagated, and the previous store insns will be removed.
*/
static void klr_const_copy_prop_pass(KlrFunc *fn, void *ctx)
{
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        klr_clear_local_var_map(bb);

        QUEUE(wklist);

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            queue_push(&wklist, insn);
        }

        while (!queue_empty(&wklist)) {
            KlrInsn *insn = queue_pop(&wklist);
            do_propagate(insn, fn, &wklist);
            do_fold(insn, fn, &wklist);
        }
    }
}

KlrPass const_copy_prop_pass = {
    .name = "const_copy_propagation",
    .callback = klr_const_copy_prop_pass,
};

#ifdef __cplusplus
}
#endif
