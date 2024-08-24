/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void update_target_block(KlrBasicBlock *bb, KlrBasicBlock *target)
{
    ASSERT(bb->num_outedges == 1);
    KlrEdge *edge = edge_out_first(bb);
    klr_remove_edge(edge);

    KlrBasicBlock *src;
    KlrUse *use, *next;
    use_foreach_safe(use, next, bb) {
        log_info("update instructions use-def chain:");
        log_info("%%%s -->> %%%s", klr_block_name(bb), klr_block_name(target));
        list_remove(&use->use_link);
        list_push_back(&target->use_list, &use->use_link);
        use->ref = (KlrValue *)target;
        src = use->insn->bb;
        edge_out_foreach(edge, src) {
            if (edge->dst == bb) {
                klr_remove_edge(edge);
                break;
            }
        }
        klr_link_edge(src, target);
    }
}

void klr_remove_only_jump_block(KlrFunc *func)
{
    /* remove block:
     * the block has only one unconditional jump
     * update all predecessor jumpers directly jump into its successor
     * update edges
     * NOTE: this pass must be run out of ssa.
     */
    KlrBasicBlock *bb, *nxt_bb;
    basic_block_foreach_safe(bb, nxt_bb, func) {
        if (bb->num_insns > 1) continue;
        if (bb->num_insns == 0) {
            log_info("delete empty basic block '%%%s'", klr_block_name(bb));
            klr_delete_block(bb);
            continue;
        }

        KlrInsn *insn = insn_first(bb);

        if (insn->flags & KLR_INSN_FLAGS_LOOP) {
            log_info("keep loop jump basic block, '%%%s'!", klr_block_name(bb));
            continue;
        }

        if (insn->code == OP_JMP) {
            log_info("only one jump in block: '%%%s'", klr_block_name(bb));
            KlrBasicBlock *target = (KlrBasicBlock *)insn->opers[0].use.ref;
            ASSERT(target->kind == KLR_VALUE_BLOCK);
            update_target_block(bb, target);
            klr_delete_insn(insn);
            klr_delete_block(bb);
        }
    }
}

static void visit_block(KlrBasicBlock *bb)
{
    KlrEdge *edge;
    edge_out_foreach(edge, bb) {
        edge->dst->visited = 1;
        visit_block(edge->dst);
    }
}

static void delete_unused_block(KlrFunc *fn)
{
    Vector unused;
    vector_init_ptr(&unused);

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        if (!bb->visited) {
            log_info("warn: bb: %s is unreachable\n", klr_block_name(bb));
            vector_push_back(&unused, &bb);
        }
        bb->visited = 0;
    }

    KlrBasicBlock **p_bb;
    vector_foreach(p_bb, &unused) {
        klr_delete_block(*p_bb);
    }

    vector_fini(&unused);
}

void klr_remove_unused_block(KlrFunc *func)
{
    // visit blocks from start block
    KlrBasicBlock *sbb = func->sbb;
    visit_block(sbb);

    // delete unused blocks
    delete_unused_block(func);
}

#ifdef __cplusplus
}
#endif
