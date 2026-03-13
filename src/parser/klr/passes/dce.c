/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static int klr_has_side_effect(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_SET_GLOBAL:
        case OP_RETURN:
        case OP_RETURN_NONE:
        case OP_IR_JMP_COND:
        case OP_JMP:
            return 1;

        case OP_CALL: {
            if (insn->flags & KLR_INSN_FLAGS_CONST) {
                KlrValue *callee = insn_oper_value(insn, 0);
                if (callee->kind == KLR_VALUE_KLASS) {
                    return 0;
                }
            }
            return 1;
        }

        case OP_MOVE: {
            KlrValue *_dst = insn_oper_value(insn, 0);
            KlrValue *src = insn_oper_value(insn, 1);
            ASSERT(klr_is_local(_dst));
            KlrInsn *dst = (KlrInsn *)_dst;

            /*
             * STRATEGY: Determine if this MOVE has any meaningful side-effects.
             * If it returns 0, the instruction is "Dead" and can be erased.
             */

            /*
             * RULE A: Global Dead Store Check
             * If the target local variable has a use_count of exactly 1,
             * it means ONLY this MOVE instruction is referencing it.
             * No other instructions (print, add, branch) are reading it.
             * Therefore, the assignment is a dead store and has no side-effects.
             * Both let and var locals can be optimized in this case.
             */
            if (dst->use_count == 1) {
                return 0; /* No side-effect: Erase the MOVE */
            }

            /*
             * RULE B: Immutable 'let' Propagation Check
             * If the target is a 'let' (Immutable), the 'const_copy_propagation'
             * pass has already broadcasted the 'src' value to all downstream users via
             * RAUW.
             */
            if (dst->flags & KLR_INSN_FLAGS_CONST) {
                /*
                 * Case B.1: Constant Propagation.
                 * let x = 10; All 'x' are now replaced by '10'.
                 */
                if (klr_is_const(src)) {
                    return 0; /* Truth already broadcasted: Erase the MOVE */
                }

                /*
                 * Case B.2: Copy Propagation.
                 * let a = b; All 'a' are now replaced by 'b'.
                 * Note: We only do this if 'src' is also a valid local/let.
                 */
                if (src->kind == KLR_VALUE_INSN &&
                    (((KlrInsn *)src)->flags & KLR_INSN_FLAGS_CONST)) {
                    return 0; /* Alias already broadcasted: Erase the MOVE */
                }
            }

            /*
             * DEFAULT: The variable is either a 'var' being read later,
             * or a 'let' that hasn't been successfully propagated yet.
             */
            return 1; /* Keep the instruction */
        }

        default:
            return 0;
    }
}

/*
Dead code elimination pass, remove instructions that have no uses.
*/
static void klr_dce_pass(KlrFunc *fn, void *ctx)
{
    log_info("perform dead code elimination on function '%%%s'", fn->name);

    int changed = 1;
    while (changed) {
        changed = 0;
        KlrBasicBlock *bb;
        basic_block_foreach(bb, fn) {
            KlrInsn *insn, *next;
            insn_foreach_safe(insn, next, bb) {
                if (!klr_value_used(insn) && !klr_has_side_effect(insn)) {
                    ASSERT(insn->use_count == 0);
                    log_info("remove dead insn:");
                    log_insn(insn);
                    klr_erase_insn(insn);
                    changed = 1;
                }
            }
        }
    }
}

KlrPass dce_pass = {
    .name = "dce",
    .callback = klr_dce_pass,
};

#ifdef __cplusplus
}
#endif
