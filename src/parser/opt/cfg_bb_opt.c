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

int klr_bb_branch_folding(KlrFunc *fn, void *data)
{
    log_info("[branch-folding] on func '%%%s'", fn->name);

    int changed = 0;

    // branch folding
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn = insn_last(bb);
        if (!insn || insn->code != OP_IR_JMP_COND) continue;

        KlrValue *cond = insn_oper_value(insn, 0);
        if (!klr_is_const(cond)) continue;

        KlrConst *konst = (KlrConst *)cond;
        ASSERT(konst->which == CONST_BOOL);
        int val = konst->bval;
        KlrBasicBlock *dst;
        if (val) {
            dst = insn_oper_value_as_bb(insn, 2);
        } else {
            dst = insn_oper_value_as_bb(insn, 3);
        }
        ASSERT(dst->kind == KLR_VALUE_BLOCK);

        log_info("branch folding: '%%%s' -->> '%%%s'", klr_block_name(bb), klr_block_name(dst));

        // remove all edges from bb to its successors
        klr_remove_all_out_edges(bb);

        // erase the conditional jump instruction
        klr_erase_insn(insn);

        // add an unconditional jump to the target block
        KlrBuilder bldr;
        klr_builder_end(&bldr, bb);
        klr_build_jmp(&bldr, dst);

        changed = 1;
    }

    return changed;
}

int klr_remove_unused_block(KlrFunc *fn, void *data)
{
    log_info("[removing-unused-block] on func '%%%s'", fn->name);

    int changed = 0;

    // clear visited flag
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    // visit all reachable blocks from sbb

    QUEUE(wklist);

    KlrBasicBlock *sbb = fn->sbb;
    sbb->visited = 1;
    queue_push(&wklist, sbb);

    while (!queue_empty(&wklist)) {
        KlrBasicBlock *bb = queue_pop(&wklist);

        KlrBasicBlock *succ;
        bb_succ_foreach(succ, bb) {
            if (succ == fn->ebb) continue;
            if (!succ->visited) {
                succ->visited = 1;
                queue_push(&wklist, succ);
            }
        }
    }

    // clear visited flag and delete unused blocks
    KlrBasicBlock *nxt;
    basic_block_foreach_safe(bb, nxt, fn) {
        if (!bb->visited) {
            log_info("basic-block: '%s' is unreachable", klr_block_name(bb));
            klr_erase_block(bb);
            changed = 1;
        }
        bb->visited = 0;
    }

    sbb->visited = 0;

    return changed;
}

// only-jmp bb MUST NOT be phi's target.
int klr_remove_only_jump_block(KlrFunc *func, void *data)
{
    log_info("[removing-only-jump-block] on func '%%%s'", func->name);

    int changed = 0;

    /* remove block:
     * the block has only one unconditional jump
     * update all predecessor jumpers directly jump into its successor
     */
    KlrBasicBlock *bb, *nxt_bb;
    basic_block_foreach_safe(bb, nxt_bb, func) {
        if (bb->num_insns != 1) continue;
        KlrInsn *insn = insn_first(bb);
        if (insn->code != OP_JMP) continue;

        log_info("only one jump in block: '%%%s'", klr_block_name(bb));

        KlrValue *_dst = insn_oper_value(insn, 0);
        if (_dst == (KlrValue *)bb) continue;

        ASSERT(_dst->kind == KLR_VALUE_BLOCK);
        KlrBasicBlock *dst = (KlrBasicBlock *)_dst;

        int has_phi_from_self = 0;
        KlrInsn *phi_insn;
        insn_foreach(phi_insn, dst) {
            if (phi_insn->code != OP_IR_PHI) break;
            for (int i = 0; i < phi_insn->filled; i++) {
                if (phi_insn->phi_preds[i] == bb) {
                    has_phi_from_self = 1;
                    break;
                }
            }
            if (has_phi_from_self) break;
        }

        if (has_phi_from_self) {
            log_info("skip removing '%s' because successor '%s' has PHI from this block",
                     klr_block_name(bb), klr_block_name(dst));
            continue;
        }

        log_info("removing only jump block: '%%%s' -> '%%%s'", klr_block_name(bb),
                 klr_block_name(dst));

        /* A -> B -> C */
        insn_foreach(insn, dst) {
            if (insn->code != OP_IR_PHI) break;

            for (int i = 0; i < insn->filled; i++) {
                if (insn->phi_preds[i] != bb) continue;

                // [Critical Assertion]: If C's Phi points to the empty block B,
                // B MUST have exactly one predecessor! Otherwise, B cannot provide
                // a single deterministic value to C, which violates the SSA property.
                ASSERT(bb->num_inedges == 1);

                KlrEdge *in_edge = edge_in_first(bb);
                KlrBasicBlock *pred = in_edge->src;

                log_info("[empty-block-remove] remap phi pred from '%%%s' to '%%%s'",
                         klr_block_name(bb), klr_block_name(pred));

                // Redirect the incoming block of C's Phi from B to A
                insn->phi_preds[i] = pred;
            }
        }

        KlrUse *use, *nxt;
        use_foreach_safe(use, nxt, bb) {
            log_info("update bb def-use chain:");
            KlrOper *oper = use->oper;
            log_info("update operand in insn:");
            log_insn(use->insn);
            set_operand(oper, use->insn, _dst);
            log_info("after update operand:");
            log_insn(use->insn);
            log_info("add edge '%%%s' -->> '%%%s'", klr_block_name(use->insn->bb),
                     klr_block_name(dst));
            klr_link_edge(use->insn->bb, dst);
        }

        if (bb->use_count == 0) {
            KlrEdge *out_edge = edge_out_first(func->sbb);
            ASSERT(out_edge);
            if (out_edge->dst == bb) {
                log_info(
                    "remove only jump block '%%%s', but it's the start block, update "
                    "func->sbb -->> '%%%s'",
                    klr_block_name(bb), klr_block_name(dst));
                klr_link_edge(func->sbb, dst);
            }
            klr_erase_block(bb);
        }

        changed = 1;
    }

    return changed;
}

int klr_merge_block(KlrFunc *fn, void *data)
{
    log_info("[basic-block-merging] on func '%%%s'", fn->name);

    int changed = 0;

    /* if A has only one out-edge and jmp to B, and B has one one in-edge and from A,
     * merge the two blocks into one.
     */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        if (bb == fn->ebb) return changed;
        if (bb->num_outedges != 1) continue;
        KlrEdge *edge = edge_out_first(bb);
        KlrBasicBlock *dst = edge->dst;
        if (dst == fn->ebb) continue;
        if (dst->num_inedges != 1) continue;

        // merge bb and dst
        log_info("merge basic block '%%%s' and '%%%s'", klr_block_name(bb), klr_block_name(dst));
        Klr_merge_block(bb, dst);
        // vector_push_back(&unused, &dst);
        changed = 1;
    }

    return changed;
}

#ifdef __cplusplus
}
#endif
