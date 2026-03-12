/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "passes.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// static void update_target_block(KlrBasicBlock *bb, KlrBasicBlock *target)
// {
//     ASSERT(bb->num_outedges == 1);
//     KlrEdge *edge = edge_out_first(bb);
//     klr_remove_edge(edge);

//     KlrBasicBlock *src;
//     KlrUse *use, *next;
//     use_foreach_safe(use, next, bb) {
//         log_info("update instructions use-def chain:");
//         log_info("%%%s -->> %%%s", klr_block_name(bb), klr_block_name(target));
//         list_remove(&use->use_link);
//         list_push_back(&target->use_list, &use->use_link);
//         target->use_count++;
//         use->ref = (KlrValue *)target;
//         src = use->insn->bb;
//         edge_out_foreach(edge, src) {
//             if (edge->dst == bb) {
//                 klr_remove_edge(edge);
//                 break;
//             }
//         }
//         klr_link_edge(src, target);
//     }
// }

// void klr_remove_only_jump_block(KlrFunc *func)
// {
//     /* remove block:
//      * the block has only one unconditional jump
//      * update all predecessor jumpers directly jump into its successor
//      * update edges
//      * NOTE: this pass must be run out of ssa.
//      */
//     KlrBasicBlock *bb, *nxt_bb;
//     basic_block_foreach_safe(bb, nxt_bb, func) {
//         if (bb->num_insns > 1) continue;
//         if (bb->num_insns == 0) {
//             log_info("delete empty basic block '%%%s'", klr_block_name(bb));
//             klr_delete_block(bb);
//             continue;
//         }

//         KlrInsn *insn = insn_first(bb);

//         if (insn->flags & KLR_INSN_FLAGS_LOOP) {
//             log_info("keep loop jump basic block, '%%%s'!", klr_block_name(bb));
//             continue;
//         }

//         if (insn->code == OP_JMP) {
//             log_info("only one jump in block: '%%%s'", klr_block_name(bb));
//             KlrBasicBlock *target = (KlrBasicBlock *)insn->opers[0].use.ref;
//             ASSERT(target->kind == KLR_VALUE_BLOCK);
//             update_target_block(bb, target);
//             klr_erase_insn(insn);
//             klr_delete_block(bb);
//         }
//     }
// }

static int klr_cf_bb_branch_folding(KlrFunc *fn)
{
    log_info("perform branch folding optimization on function '%%%s'", fn->name);

    int changed = 0;

    // branch folding
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn = insn_last(bb);
        if (!insn || insn->code != OP_IR_JMP_COND) continue;

        KlrValue *cond = insn_oper_value(insn, 0);
        if (!klr_is_const(cond)) continue;

        KlrConst *konst = klr_get_const_value(cond);
        ASSERT(konst->which == CONST_BOOL);
        int val = konst->bval;
        KlrBasicBlock *dst;
        if (val) {
            dst = (KlrBasicBlock *)insn_oper_value(insn, 1);
        } else {
            dst = (KlrBasicBlock *)insn_oper_value(insn, 2);
        }
        ASSERT(dst->kind == KLR_VALUE_BLOCK);

        log_info("branch folding: '%%%s' -->> '%%%s'", klr_block_name(bb),
                 klr_block_name(dst));

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

static int klr_cfg_remove_unused_block(KlrFunc *fn)
{
    int changed = 0;

    // clear visited flag
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    // visit all reachable blocks from sbb

    QUEUE(worklist);

    KlrBasicBlock *sbb = fn->sbb;
    sbb->visited = 1;
    queue_push(&worklist, sbb);

    while (!queue_empty(&worklist)) {
        KlrBasicBlock *bb = queue_pop(&worklist);

        KlrEdge *edge;
        edge_out_foreach(edge, bb) {
            KlrBasicBlock *dst = edge->dst;
            if (dst == fn->ebb) continue;
            if (!dst->visited) {
                dst->visited = 1;
                queue_push(&worklist, dst);
            }
        }
    }

    // clear visited flag and delete unused blocks
    KlrBasicBlock *nxt;
    basic_block_foreach_safe(bb, nxt, fn) {
        if (!bb->visited) {
            log_info("basic-block: '%s' is unreachable\n", klr_block_name(bb));
            klr_delete_block(bb);
            changed = 1;
        }
        bb->visited = 0;
    }

    sbb->visited = 0;

    return changed;
}

static int klr_cf_merge_block(KlrFunc *fn)
{
    log_info("perform basic block merging optimization on function '%%%s'", fn->name);

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
        log_info("merge basic block '%%%s' and '%%%s'", klr_block_name(bb),
                 klr_block_name(dst));
        Klr_merge_block(bb, dst);
        // vector_push_back(&unused, &dst);
        changed = 1;
    }

    return changed;
}

void klr_cfg_bb_opt_pass(KlrFunc *fn, void *ctx)
{
    int changed = 1;
    while (changed) {
        changed = 0;
        changed |= klr_cf_bb_branch_folding(fn);
        changed |= klr_cfg_remove_unused_block(fn);
        changed |= klr_cf_merge_block(fn);
    }
}

void register_cfg_bb_opt_pass(KlrPassGroup *grp)
{
    klr_add_pass(grp, "cfg_bb_opt", klr_cfg_bb_opt_pass, NULL);
}

#ifdef __cplusplus
}
#endif
