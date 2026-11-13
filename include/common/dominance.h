/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_DOMINANCE_H_
#define _KOALA_DOMINANCE_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Dominator tree and dominance frontiers of a directed graph, computed with
 *
 *   Keith D. Cooper, Timothy J. Harvey, Ken Kennedy,
 *   "A Simple, Fast Dominance Algorithm",
 *   Software Practice & Experience, 4:1-10, 2001.
 *   https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/dom14.pdf
 *
 * The graph is abstract: nodes are `0 .. num_nodes - 1` and edges are given
 * as `(src, dst)` pairs, so it doesn't depend on any IR and works on both
 * SSA and non-SSA CFGs. Nodes unreachable from `entry` have no dominator.
 */
typedef struct _DomTree {
    /* number of nodes */
    int num_nodes;
    /* entry node */
    int entry;
    /* number of nodes reachable from entry */
    int num_reachable;
    /* reachable nodes in reverse post-order, rpo[0] == entry */
    int *rpo;
    /* node -> its index in `rpo`, -1 if unreachable */
    int *rpo_index;
    /* node -> immediate dominator, idom[entry] == entry, -1 if unreachable */
    int *idom;
    /* node -> depth in the dominator tree, entry is 0, -1 if unreachable */
    int *depth;
    /* dominator tree preorder enter/exit numbers, for O(1) dominates */
    int *dfs_in;
    int *dfs_out;
    /* dominator tree children in CSR form: children of `n` are
       child_list[child_start[n] .. child_start[n + 1]) */
    int *child_start;
    int *child_list;
    /* dominance frontiers in CSR form, same layout as children */
    int *df_start;
    int *df_list;
    /* passes of the paper Figure 3 loop, including the final pass that
       changes nothing (the paper counts iterations the same way) */
    int num_iterations;
} DomTree;

/*
 * Build the dominator tree and dominance frontiers.
 * `edges` holds `num_edges` pairs: edges[2*i] -> edges[2*i+1].
 * Duplicate edges and self loops are allowed.
 * Returns 0 on success, -1 on invalid arguments.
 */
int dom_tree_build(DomTree *dt, int num_nodes, int entry, const int *edges, int num_edges);

/* Release the memory of the dominator tree. */
void dom_tree_fini(DomTree *dt);

/* Immediate dominator of `n`, `entry` for entry itself, -1 if unreachable. */
static inline int dom_idom(DomTree *dt, int n) { return dt->idom[n]; }

/* Is `n` reachable from entry? */
static inline int dom_reachable(DomTree *dt, int n) { return dt->rpo_index[n] >= 0; }

/* Does `a` dominate `b`? Every node dominates itself. O(1). */
int dom_dominates(DomTree *dt, int a, int b);

/* Does `a` strictly dominate `b`? */
static inline int dom_strictly_dominates(DomTree *dt, int a, int b)
{
    return a != b && dom_dominates(dt, a, b);
}

/* Children of `n` in the dominator tree. */
static inline int dom_num_children(DomTree *dt, int n)
{
    return dt->child_start[n + 1] - dt->child_start[n];
}

static inline int *dom_children(DomTree *dt, int n) { return dt->child_list + dt->child_start[n]; }

/* Dominance frontier of `n`, in ascending node order. */
static inline int dom_num_frontier(DomTree *dt, int n)
{
    return dt->df_start[n + 1] - dt->df_start[n];
}

static inline int *dom_frontier(DomTree *dt, int n) { return dt->df_list + dt->df_start[n]; }

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DOMINANCE_H_ */
