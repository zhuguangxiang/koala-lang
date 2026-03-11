/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
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
            return 1;
        case OP_CALL: {
            if (insn->flags & KLR_INSN_FLAGS_CONST) {
                return 0;
            } else {
                return 1;
            }
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
    int changed = 1;
    while (changed) {
        changed = 0;
        KlrBasicBlock *bb;
        basic_block_foreach(bb, fn) {
            KlrInsn *insn, *next;
            insn_foreach_safe(insn, next, bb) {
                if (!klr_value_used(insn) && !klr_has_side_effect(insn)) {
                    klr_erase_insn(insn);
                    changed = 1;
                }
            }
        }
    }
    printf("\n");
}

void register_dce_pass(KlrPassGroup *grp)
{
    klr_add_pass(grp, "dce", klr_dce_pass, NULL);
}

#ifdef __cplusplus
}
#endif
