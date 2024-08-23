/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

// update the insn all Uses and remove it.
static void update_load_insn_uses(KlrInsn *insn)
{
    /* update instructions who use this instruction */
    KlrUse *use, *nxt;
    ASSERT(insn->num_opers == 1);
    KlrValue *val = insn->opers[0].use.ref;
    ASSERT(val->kind == KLR_VALUE_LOCAL || val->kind == KLR_VALUE_PARAM);
    use_foreach_safe(use, nxt, insn) {
        /* remove use from current insn */
        list_remove(&use->use_link);
        use->ref = (KlrValue *)val;
        list_push_back(&val->use_list, &use->use_link);

        /* update operand as local or param */
        if (val->kind == KLR_VALUE_LOCAL) {
            use->oper->kind = KLR_OPER_LOCAL;
        } else if (val->kind == KLR_VALUE_PARAM) {
            use->oper->kind = KLR_OPER_PARAM;
        } else {
            NIY();
        }
    }
    klr_delete_insn(insn);
}

/*
 * remove all load insns
 * If all operands that are load insns, update these operands point to the variable.
 */
void klr_remove_load_pass(KlrFunc *func, void *ctx)
{
    KlrBasicBlock *bb;
    KlrInsn *insn, *nxt_insn;
    basic_block_foreach(bb, func) {
        // update all insns that use load insn
        insn_foreach_safe(insn, nxt_insn, bb) {
            if (insn->code == OP_IR_LOAD) {
                update_load_insn_uses(insn);
            }
        }
    }
}

/*
 * remove unused store insns
 * If there is no load insn in two store insns, remove the first store insn.
 */
void klr_remove_store_pass(KlrFunc *func, void *ctx)
{
    KlrLocal **p_var;
    local_foreach(p_var, func) {
        KlrLocal *var = *p_var;
        KlrUse *use, *nxt;
        KlrInsn *prev = NULL;
        use_foreach_safe(use, nxt, var) {
            KlrInsn *insn = use->insn;
            if (insn->code == OP_IR_STORE) {
                if (prev) {
                    // remove unused store insns
                    klr_delete_insn(prev);
                }
                prev = insn;
            } else {
                prev = NULL;
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
