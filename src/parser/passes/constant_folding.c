/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

static int try_arith_compute(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_BINARY_ADD: {
            KlrOper *oper1 = &insn->opers[0];
            KlrOper *oper2 = &insn->opers[1];
            if (oper1->kind == KLR_OPER_CONST && oper2->kind == KLR_OPER_CONST) {
                KlrConst *lhs = (KlrConst *)oper1->use.ref;
                KlrConst *rhs = (KlrConst *)oper2->use.ref;
                if (lhs->which == CONST_INT && rhs->which == CONST_INT) {
                    int64_t val = lhs->ival + rhs->ival;
                    insn->result = klr_const_int(val);
                }
                return 1;
            }
            return 0;
        }
        case OP_BINARY_SUB: {
            return 0;
        }
        default: {
            /* not arithmetic operation */
            return 0;
        }
    }
}

// update the insn all Uses and remove it.
static void eliminate_constant_insn(KlrInsn *insn)
{
    /* update instructions who use this instruction */
    KlrUse *use, *nxt;
    KlrInsn *ref;
    KlrValue *val = insn->result;
    use_foreach_safe(use, nxt, insn) {
        /* remove use from current insn */
        list_remove(&use->use_link);
        use->ref = (KlrValue *)val;
        list_push_back(&val->use_list, &use->use_link);

        /* update operand as constant */
        use->oper->kind = KLR_OPER_CONST;
    }
    klr_delete_insn(insn);
}

/*
 * constant folding:
 * if all operands of this insn are constant and the result of this insn can be
 * computed. And this insn is called 'constant instruction'.
 * Here is arithmetic operation.
 */
void klr_constant_folding_pass(KlrFunc *func, void *ctx)
{
    KlrBasicBlock *bb;
    KlrInsn *insn, *nxt_insn;
    basic_block_foreach(bb, func) {
        insn_foreach_safe(insn, nxt_insn, bb) {
            if (try_arith_compute(insn)) {
                eliminate_constant_insn(insn);
            }
        }
    }
}

void register_constant_folding_pass(KlrPassGroup *grp)
{
    klr_add_pass(grp, "constant_folding", klr_constant_folding_pass, NULL);
}

/*
 * constant propagation:
 * Def-Value: one store insn gives the variable's Def-Value.
 * If the Def-Value is constant, traverse the Def-Value valid interval,
 * update all Use-Values.
 */
void klr_constant_propagation_pass(KlrFunc *func, void *ctx)
{
    KlrLocal **p_var;
    local_foreach(p_var, func) {
        KlrLocal *var = *p_var;
        KlrUse *use, *nxt;
        KlrValue *const_value = NULL;
        use_foreach_safe(use, nxt, var) {
            KlrInsn *insn = use->insn;
            if (insn->code == OP_IR_STORE) {
                KlrValue *val = insn->opers[1].use.ref;
                if (val->kind == KLR_VALUE_CONST) {
                    const_value = val;
                } else {
                    const_value = NULL;
                }
            } else {
                if (const_value) {
                    list_remove(&use->use_link);
                    list_push_back(&const_value->use_list, &use->use_link);
                    use->ref = const_value;
                    use->oper->kind = KLR_OPER_CONST;
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
