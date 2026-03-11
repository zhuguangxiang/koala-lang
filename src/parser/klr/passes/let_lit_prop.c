/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "passes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
The `let` variable is immutable, so it can be propated.
This is a global constant propagation and no need SSA format.
It can be used to fold list/tuple/map/set literals, and also can be used to fold const
variables.
*/
static void klr_let_lit_prop_pass(KlrFunc *fn, void *ctx)
{
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            OpCode op = insn->code;
            switch (op) {
                case OP_CONST: {
                    // do nothing
                    break;
                }

                case OP_SET_GLOBAL: {
                    KlrGlobal *global = (KlrGlobal *)insn_oper_value(insn, 0);
                    if (!global->mutable) {
                        KlrValue *val = insn_oper_value(insn, 1);
                        if (klr_is_const(val)) {
                            KlrConst *kval = klr_get_const_value(val);
                            update_insn_operand(insn, 1, (KlrValue *)kval);
                            global->kval = kval;
                        }
                    }
                    break;
                }

                case OP_GET_GLOBAL: {
                    KlrGlobal *global = (KlrGlobal *)insn_oper_value(insn, 0);
                    if (!global->mutable) {
                        KlrConst *val = global->kval;
                        if (val) {
                            kl_replace_all_uses_with((KlrValue *)val, (KlrValue *)insn);
                        }
                    }
                    break;
                }

                case OP_CALL: {
                    if (insn->flags & KLR_INSN_FLAGS_CONST) {
                        KlrKlass *callee = (KlrKlass *)insn_oper_value(insn, 0);
                        if (callee->kind == KLR_VALUE_KLASS) {
                            KlrValue *items[insn->num_opers - 1];
                            memset(items, 0, sizeof(items));
                            // skip callee operand
                            KlrValue *val;
                            oper_value_foreach(val, insn, 1) {
                                ASSERT(klr_is_const(val));
                                KlrConst *kval = klr_get_const_value(val);
                                items[i__ - 1] = (KlrValue *)kval;
                            }

                            if (!strcmp(callee->name, "list")) {
                                val = klr_const_list(items, insn->num_opers - 1, insn->ts,
                                                     fn->mod);
                            } else if (!strcmp(callee->name, "tuple")) {
                                val = klr_const_tuple(items, insn->num_opers - 1,
                                                      insn->ts, fn->mod);
                            } else if (!strcmp(callee->name, "int64")) {
                                ASSERT(insn->num_opers == 2);
                                val = insn_oper_value(insn, 1);
                                ASSERT(klr_is_const(val));
                            } else {
                                printf("unsupported const call to class '%s'\n",
                                       callee->name);
                                NYI();
                            }

                            kl_replace_all_uses_with(val, (KlrValue *)insn);
                        }
                    }
                    break;
                }

                case OP_BINARY_ADD:
                case OP_BINARY_SUB: {
                    KlrValue *lhs = insn_oper_value(insn, 0);
                    KlrValue *rhs = insn_oper_value(insn, 1);
                    if (klr_is_const(lhs) && klr_is_const(rhs)) {
                        KlrConst *lval = klr_get_const_value(lhs);
                        KlrConst *rval = klr_get_const_value(rhs);
                        if (lval->which == CONST_INT && rval->which == CONST_INT) {
                            uint64_t res = 0;
                            if (op == OP_BINARY_ADD) {
                                res = lval->ival + rval->ival;
                            } else {
                                res = lval->ival - rval->ival;
                            }
                            KlrValue *const_res = klr_const_int(res, lval->ts, fn->mod);
                            kl_replace_all_uses_with(const_res, (KlrValue *)insn);
                        }
                    }
                    break;
                }

                case OP_MOVE: {
                    ASSERT(!klr_value_used(insn));
                    KlrValue *dst = insn_oper_value(insn, 0);
                    KlrValue *src = insn_oper_value(insn, 1);
                    if (klr_is_local(dst)) {
                        KlrInsn *dst_insn = (KlrInsn *)dst;
                        // local is let and src is constant, propagate src to dst and
                        // delete this move insn in global level and needn't SSA.
                        if (dst_insn->flags & KLR_INSN_FLAGS_CONST) {
                            if (klr_is_const(src)) {
                                kl_replace_all_uses_with(src, (KlrValue *)dst_insn);
                            }
                        }
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
}

void register_let_lit_prop_pass(KlrPassGroup *grp)
{
    klr_add_pass(grp, "let_literal_propagation", klr_let_lit_prop_pass, NULL);
}

#ifdef __cplusplus
}
#endif
