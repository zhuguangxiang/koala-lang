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

    QUEUE(wklist);

    KlrBasicBlock *sbb = fn->sbb;
    sbb->visited = 1;
    queue_push(&wklist, sbb);

    while (!queue_empty(&wklist)) {
        KlrBasicBlock *bb = queue_pop(&wklist);

        KlrEdge *edge;
        edge_out_foreach(edge, bb) {
            KlrBasicBlock *dst = edge->dst;
            if (dst == fn->ebb) continue;
            if (!dst->visited) {
                dst->visited = 1;
                queue_push(&wklist, dst);
            }
        }
    }

    // clear visited flag and delete unused blocks
    KlrBasicBlock *nxt;
    basic_block_foreach_safe(bb, nxt, fn) {
        if (!bb->visited) {
            log_info("basic-block: '%s' is unreachable", klr_block_name(bb));
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
