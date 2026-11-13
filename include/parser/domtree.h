/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_DOMTREE_H_
#define _KOALA_DOMTREE_H_

#include "dominance.h"
#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Dominator tree of a KLR function, a thin adapter over common/dominance.c
 * (Cooper, Harvey, Kennedy, "A Simple, Fast Dominance Algorithm", 2001).
 * https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/dom14.pdf
 *
 * It only looks at the CFG, so it works on both the default (non-SSA) IR and
 * SSA IR. The nodes are `sbb`, every block in `bb_list` and `ebb`, the entry
 * is `sbb`. The tree is a snapshot: it is stale once the CFG changes, rebuild
 * it after adding or removing blocks or edges.
 */
typedef struct _KlrDomTree {
    KlrFunc *fn;
    /* dom index -> block */
    int num_blocks;
    KlrBasicBlock **blocks;
    DomTree dt;
} KlrDomTree;

/* Build the dominator tree of `fn`, sets `bb->dom_index` of every block. */
void klr_domtree_build(KlrDomTree *kdt, KlrFunc *fn);

void klr_domtree_fini(KlrDomTree *kdt);

/* Is `bb` reachable from the start block? */
int klr_reachable(KlrDomTree *kdt, KlrBasicBlock *bb);

/* Immediate dominator, NULL for the start block and unreachable blocks. */
KlrBasicBlock *klr_idom(KlrDomTree *kdt, KlrBasicBlock *bb);

/* Does `a` dominate `b`? A block dominates itself. O(1). */
int klr_dominates(KlrDomTree *kdt, KlrBasicBlock *a, KlrBasicBlock *b);

/* A back edge jumps to a block dominating its source (a natural loop). */
int klr_is_back_edge(KlrDomTree *kdt, KlrEdge *edge);

/* Print the tree, dominance frontiers and back edges. */
void klr_domtree_print(KlrDomTree *kdt, FILE *fp);

/* Free `fn->domtree`, e.g. after a pass changes the CFG. */
void klr_func_free_domtree(KlrFunc *fn);

/* --domtree pass: build `fn->domtree` of every function, replacing a stale one, and print
   them with --dump=domtree. */
void kl_domtree(KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DOMTREE_H_ */
