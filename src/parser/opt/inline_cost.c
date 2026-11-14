/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "inline_cost.h"
#include "domtree.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GCC-inspired model tuned for Koala; one size unit is one KLR instruction. */
#define RECURSIVE_INLINE_MAX_SIZE        4096
#define RECURSIVE_INLINE_MAX_BLOCKS      1024
#define RECURSIVE_INLINE_MIN_PROBABILITY 50

void recursive_inline_cost_init(RecursiveInlineCost *cost, int insns, int returns, int params,
                                int blocks)
{
    ASSERT(insns >= 0 && returns >= 0 && params >= 0 && blocks > 0);
    cost->size = insns;
    cost->size_per_copy = insns + returns + 2LL * params + 2;
    cost->blocks = blocks;
    cost->blocks_per_copy = blocks + 1;
}

int recursive_inline_cost_allow_level(RecursiveInlineCost *cost, int depth, int calls)
{
    ASSERT(depth > 0 && calls > 0);
    long long size = cost->size + calls * cost->size_per_copy;
    long long blocks = cost->blocks + (long long)calls * cost->blocks_per_copy;
    if (depth > RECURSIVE_INLINE_MAX_DEPTH || size > RECURSIVE_INLINE_MAX_SIZE ||
        blocks > RECURSIVE_INLINE_MAX_BLOCKS)
        return 0;
    cost->size = size;
    cost->blocks = (int)blocks;
    return 1;
}

static int reachable_without_edge(KlrDomTree *dom, KlrBasicBlock *target, KlrEdge *excluded)
{
    int seen[dom->num_blocks];
    KlrBasicBlock *queue[dom->num_blocks];
    memset(seen, 0, sizeof(seen));
    int head = 0, tail = 0;
    queue[tail++] = dom->fn->sbb;
    seen[dom->fn->sbb->dom_index] = 1;
    while (head < tail) {
        KlrBasicBlock *bb = queue[head++];
        if (bb == target) return 1;
        KlrEdge *edge;
        edge_out_foreach(edge, bb) {
            if (edge == excluded || seen[edge->dst->dom_index]) continue;
            seen[edge->dst->dom_index] = 1;
            queue[tail++] = edge->dst;
        }
    }
    return 0;
}

void recursive_inline_call_probabilities(KlrFunc *fn, KlrInsn **calls, int count,
                                         int *probabilities)
{
    KlrDomTree dom;
    klr_domtree_build(&dom, fn);
    for (int i = 0; i < count; i++) {
        KlrBasicBlock *target = calls[i]->bb;
        int probability = RECURSIVE_INLINE_PROB_SCALE;
        if (!klr_reachable(&dom, target)) {
            probabilities[i] = 0;
            continue;
        }
        KlrBasicBlock *bb;
        basic_block_foreach(bb, fn) {
            KlrInsn *branch = insn_last(bb);
            if (!branch || !insn_is(branch, OP_IR_JMP_COND) || !klr_dominates(&dom, bb, target))
                continue;
            KlrBasicBlock *yes = (KlrBasicBlock *)insn_oper_value(branch, 2);
            KlrBasicBlock *no = (KlrBasicBlock *)insn_oper_value(branch, 3);
            if (yes == no) continue;
            KlrEdge *yes_edge = klr_find_edge(bb, yes);
            KlrEdge *no_edge = klr_find_edge(bb, no);
            ASSERT(yes_edge && no_edge);
            if (!reachable_without_edge(&dom, target, yes_edge) ||
                !reachable_without_edge(&dom, target, no_edge))
                probability /= 2;
        }
        probabilities[i] = probability;
    }
    klr_domtree_fini(&dom);
}

int recursive_inline_probability_product(int parent, int callee)
{
    ASSERT(parent >= 0 && parent <= RECURSIVE_INLINE_PROB_SCALE);
    ASSERT(callee >= 0 && callee <= RECURSIVE_INLINE_PROB_SCALE);
    return parent * callee / RECURSIVE_INLINE_PROB_SCALE;
}

int recursive_inline_probability_allowed(int probability)
{
    ASSERT(probability >= 0 && probability <= RECURSIVE_INLINE_PROB_SCALE);
    return probability > RECURSIVE_INLINE_MIN_PROBABILITY;
}

#ifdef __cplusplus
}
#endif
