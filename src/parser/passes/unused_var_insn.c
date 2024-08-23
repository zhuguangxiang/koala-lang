/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

static char *name_or_tag(KlrValue *val)
{
    if (val->name[0]) return val->name;
    static char sbuf[32];
    sprintf(sbuf, "%%%d", val->tag);
    return sbuf;
}

/* remove unused insns and variables */
void klr_remove_unused_pass(KlrFunc *func, void *ctx)
{
    KlrLocal **p_var;
    local_foreach(p_var, func) {
        KlrLocal *var = *p_var;
        KlrUse *use;
        int used = 0;
        use_foreach(use, var) {
            KlrInsn *insn = use->insn;
            if (insn->code != OP_IR_STORE) {
                used = 1;
                break;
            }
        }

        if (!used) {
            printf("var %s is not used\n", name_or_tag((KlrValue *)var));
        }
    }

    // KlrBasicBlock *bb;
    // KlrInsn *insn, *nxt_insn;
    // basic_block_foreach(bb, func) {
    //     insn_foreach_safe(insn, nxt_insn, bb) {
    //         if (list_empty(&insn->use_list)) {
    //             printf("insn %s is not used\n", name_or_tag((KlrValue *)insn));
    //         }
    //     }
    // }
}

#ifdef __cplusplus
}
#endif
