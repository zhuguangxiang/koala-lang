/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "passes.h"

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
            // local is let and src is constant
            if (dst->flags & KLR_INSN_FLAGS_CONST) {
                if (klr_is_const(src) ||
                    (src->kind == KLR_VALUE_INSN &&
                     (((KlrInsn *)src)->flags & KLR_INSN_FLAGS_CONST))) {
                    return 0;
                }
            } else {
                // local is var
                if (dst->use_count == 1) {
                    return 0;
                }
            }
            return 1;
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

void register_dce_pass(KlrPassGroup *grp)
{
    klr_add_pass(grp, "dce", klr_dce_pass, NULL);
}

#ifdef __cplusplus
}
#endif
