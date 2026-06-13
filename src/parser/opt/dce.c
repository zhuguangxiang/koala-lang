/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "opt.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

static int has_side_effect(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_GLOBAL_SET:
        case OP_RET:
        case OP_RET_VOID:
        case OP_IR_JMP_COND:
        case OP_JMP:
        case OP_IR_LOCAL:
        case OP_NEW:
        case OP_SET_FIELD:
        case OP_SET_FIELD_EXT:
        case OP_SEQ_SET:
        case OP_MAP_SET:
        case OP_SEQ_SET_IMM:
            return 1;

        case OP_IR_CALL: {
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
             * If the target local variable has a use_count of exactly 0, it
             * means no instruction is reading from it. This MOVE is the only
             * instruction writing to it, so it has no side-effects and can be
             * safely removed.
             */
            if (dst->use_count == 0) {
                return 0; /* No side-effect: Erase the MOVE */
            }

            /*
             * RULE B: Immutable 'let' Propagation Check
             * If the target is a 'let' (Immutable), the
             * 'const_copy_propagation' pass has already broadcasted the 'src'
             * value to all downstream users via RAUW.
             */
            if (dst->flags & KLR_INSN_FLAGS_CONST) {
                /*
                 * Case B.1: Constant Propagation.
                 * let x = 10; All 'x' are now replaced by '10'.
                 */
                if (klr_is_const(src)) {
                    if (!type_allowed_to_prop(src->ts)) {
                        return 1; /* Type not allowed for propagation: Keep the MOVE for safety */
                    }
                    return 0; /* Truth already broadcasted: Erase the MOVE */
                }

                /*
                 * Case B.2: Copy Propagation.
                 * let a = b; All 'a' are now replaced by 'b'.
                 * Note: We only do this if 'src' is also a valid local/let.
                 */
                if (klr_is_local(src) && (((KlrInsn *)src)->flags & KLR_INSN_FLAGS_CONST)) {
                    return 0; /* Alias already broadcasted: Erase the MOVE */
                }

                if (src->kind == KLR_VALUE_PARAM) {
                    return 0; /* Parameter is immutable: Erase the MOVE */
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
int klr_dce_pass(KlrFunc *fn, void *data)
{
    log_info("[dead-code-elimination] on func '%%%s'", fn->name);

    QUEUE(wklist);

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            /* If no one uses it and it has no side-effects, it's a candidate */
            if (!klr_is_used(insn) && !has_side_effect(insn)) {
                queue_push(&wklist, insn);
            }
        }
    }

    int changed = 0;

    while (!queue_empty(&wklist)) {
        KlrInsn *insn = queue_pop(&wklist);

        /* In case it was already erased or its status changed */
        if (insn->use_count > 0 || has_side_effect(insn)) {
            continue;
        }

        /* Check if any of its operands become dead after this removal */
        KlrValue *val;
        insn_oper_value_foreach(val, insn) {
            /* klr_erase_insn will do use_count--, check it use_count == 1
             * and push to worklist.
             */
            if (val->kind == KLR_VALUE_INSN && (val->use_count == 1)) {
                log_info("add insn to wklist:");
                log_insn((KlrInsn *)val);
                queue_push(&wklist, val);
            }
        }

        log_info("remove dead insn:");
        log_insn(insn);

        /* remove from the IR linked list */
        klr_erase_insn(insn);

        changed = 1;
    }

    return changed;
}

#ifdef __cplusplus
}
#endif
