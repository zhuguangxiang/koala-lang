/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Dominance, after
 *
 *   Keith D. Cooper, Timothy J. Harvey, Ken Kennedy,
 *   "A Simple, Fast Dominance Algorithm",
 *   Software Practice & Experience, 4:1-10, 2001.
 *   https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/dom14.pdf
 *
 * Definitions (paper section 1):
 *   - `a` dominates `b` if every path from entry to `b` passes through `a`.
 *     Every node dominates itself.
 *   - idom(b), the immediate dominator, is the unique strict dominator of `b`
 *     that every other strict dominator of `b` also dominates. The idom
 *     relation forms the dominator tree, rooted at entry.
 *   - The dominance frontier DF(a) is the set of nodes `b` such that `a`
 *     dominates a predecessor of `b` but does not strictly dominate `b`.
 *
 * Dominator tree (paper section 3, Figure 3):
 *
 *   for all nodes, b             // initialize the dominators array
 *       doms[b] <- Undefined
 *   doms[start_node] <- start_node
 *   Changed <- true
 *   while (Changed)
 *       Changed <- false
 *       for all nodes, b, in reverse postorder (except start_node)
 *           new_idom <- first (processed) predecessor of b
 *           for all other predecessors, p, of b
 *               if doms[p] != Undefined   // i.e., if doms[p] already calculated
 *                   new_idom <- intersect(p, new_idom)
 *           if doms[b] != new_idom
 *               doms[b] <- new_idom
 *               Changed <- true
 *
 *   function intersect(b1, b2) returns node
 *       finger1 <- b1
 *       finger2 <- b2
 *       while (finger1 != finger2)
 *           while (finger1 < finger2)
 *               finger1 = doms[finger1]
 *           while (finger2 < finger1)
 *               finger2 = doms[finger2]
 *       return finger1
 *
 * The paper compares postorder numbers, where entry has the largest number.
 * We compare reverse post-order numbers instead (entry is 0), so the
 * comparisons flip: walk up the finger with the larger RPO number.
 *
 * The algorithm is a data-flow solution whose `doms` array *is* the tree, so
 * it needs no bit vectors. With RPO traversal it converges in d(G) + 3 passes
 * at most, where d(G) is the loop connectedness of the graph (the maximum
 * number of back edges on any acyclic path); for reducible graphs that is
 * usually two passes. In practice it beats Lengauer-Tarjan on the graph sizes
 * seen in compilers, although its worst case is O(N^2).
 *
 * Dominance frontiers (paper section 4, Figure 5):
 *
 *   for all nodes, b
 *       if the number of predecessors of b >= 2
 *           for all predecessors, p, of b
 *               runner <- p
 *               while runner != doms[b]
 *                   add b to runner's dominance frontier set
 *                   runner = doms[runner]
 *
 * Nodes with a single predecessor `p` are skipped by the paper since then
 * doms[b] == p and the loop body never runs. We also run it for those nodes,
 * which is a no-op except for entry with incoming edges and self loops, so
 * the result matches the definition for every graph. Entry is its own idom,
 * so the walk for it stops right after entry instead of at doms[entry].
 */

#include "dominance.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compressed adjacency lists. */
typedef struct _Csr {
    int *start;
    int *list;
} Csr;

/* Build successors (by = 0) or predecessors (by = 1) of `edges`, keeping the
   input order of edges for each node. */
static void csr_build(Csr *csr, int num_nodes, const int *edges, int num_edges, int by)
{
    csr->start = mm_alloc(sizeof(int) * (num_nodes + 1));
    csr->list = mm_alloc(sizeof(int) * (num_edges + 1));

    for (int i = 0; i < num_edges; i++) csr->start[edges[2 * i + by] + 1]++;
    for (int i = 0; i < num_nodes; i++) csr->start[i + 1] += csr->start[i];

    int *pos = mm_alloc(sizeof(int) * (num_nodes + 1));
    memcpy(pos, csr->start, sizeof(int) * num_nodes);
    for (int i = 0; i < num_edges; i++) {
        int from = edges[2 * i + by];
        csr->list[pos[from]++] = edges[2 * i + 1 - by];
    }
    mm_free(pos);
}

static void csr_fini(Csr *csr)
{
    mm_free(csr->start);
    mm_free(csr->list);
}

/* Fill dt->rpo and dt->rpo_index with an iterative DFS from entry. */
static void compute_rpo(DomTree *dt, Csr *succs)
{
    int n = dt->num_nodes;
    int *post = mm_alloc(sizeof(int) * n);
    int *stack = mm_alloc(sizeof(int) * n);
    int *next = mm_alloc(sizeof(int) * n);
    char *visited = mm_alloc(n);
    int num_post = 0;
    int sp = 0;

    stack[sp++] = dt->entry;
    visited[dt->entry] = 1;
    next[dt->entry] = succs->start[dt->entry];

    while (sp > 0) {
        int node = stack[sp - 1];
        if (next[node] < succs->start[node + 1]) {
            int s = succs->list[next[node]++];
            if (!visited[s]) {
                visited[s] = 1;
                next[s] = succs->start[s];
                stack[sp++] = s;
            }
        } else {
            post[num_post++] = node;
            sp--;
        }
    }

    dt->num_reachable = num_post;
    for (int i = 0; i < n; i++) dt->rpo_index[i] = -1;
    for (int i = 0; i < num_post; i++) {
        int node = post[num_post - 1 - i];
        dt->rpo[i] = node;
        dt->rpo_index[node] = i;
    }

    mm_free(post);
    mm_free(stack);
    mm_free(next);
    mm_free(visited);
}

/* Paper Figure 3 `intersect`, on RPO numbers. */
static int intersect(DomTree *dt, int b1, int b2)
{
    int *idom = dt->idom;
    int *order = dt->rpo_index;
    while (b1 != b2) {
        while (order[b1] > order[b2]) b1 = idom[b1];
        while (order[b2] > order[b1]) b2 = idom[b2];
    }
    return b1;
}

/* Paper Figure 3 main loop. */
static void compute_idom(DomTree *dt, Csr *preds)
{
    int *idom = dt->idom;
    for (int i = 0; i < dt->num_nodes; i++) idom[i] = -1;
    idom[dt->entry] = dt->entry;

    int changed = 1;
    dt->num_iterations = 0;
    while (changed) {
        changed = 0;
        dt->num_iterations++;
        for (int i = 1; i < dt->num_reachable; i++) {
            int b = dt->rpo[i];
            int new_idom = -1;
            for (int k = preds->start[b]; k < preds->start[b + 1]; k++) {
                int p = preds->list[k];
                /* unprocessed or unreachable predecessor */
                if (idom[p] < 0) continue;
                new_idom = (new_idom < 0) ? p : intersect(dt, p, new_idom);
            }
            /* reachable non-entry node has a processed pred in RPO */
            ASSERT(new_idom >= 0);
            if (idom[b] != new_idom) {
                idom[b] = new_idom;
                changed = 1;
            }
        }
    }
}

/* Build the children lists, depths and preorder numbers of the tree. */
static void compute_tree(DomTree *dt)
{
    int n = dt->num_nodes;
    dt->child_start = mm_alloc(sizeof(int) * (n + 1));
    dt->child_list = mm_alloc(sizeof(int) * (n + 1));

    /* children in RPO order, so the walk below is deterministic */
    for (int i = 1; i < dt->num_reachable; i++) dt->child_start[dt->idom[dt->rpo[i]] + 1]++;
    for (int i = 0; i < n; i++) dt->child_start[i + 1] += dt->child_start[i];
    int *pos = mm_alloc(sizeof(int) * (n + 1));
    memcpy(pos, dt->child_start, sizeof(int) * n);
    for (int i = 1; i < dt->num_reachable; i++) {
        int b = dt->rpo[i];
        dt->child_list[pos[dt->idom[b]]++] = b;
    }
    mm_free(pos);

    for (int i = 0; i < n; i++) {
        dt->depth[i] = -1;
        dt->dfs_in[i] = -1;
        dt->dfs_out[i] = -1;
    }

    /* iterative preorder walk; `next` is the next child to visit */
    int *stack = mm_alloc(sizeof(int) * (n + 1));
    int *next = mm_alloc(sizeof(int) * (n + 1));
    int sp = 0;
    int clock = 0;

    stack[sp++] = dt->entry;
    next[dt->entry] = dt->child_start[dt->entry];
    dt->depth[dt->entry] = 0;
    dt->dfs_in[dt->entry] = clock++;

    while (sp > 0) {
        int node = stack[sp - 1];
        if (next[node] < dt->child_start[node + 1]) {
            int c = dt->child_list[next[node]++];
            dt->depth[c] = dt->depth[node] + 1;
            dt->dfs_in[c] = clock++;
            next[c] = dt->child_start[c];
            stack[sp++] = c;
        } else {
            dt->dfs_out[node] = clock++;
            sp--;
        }
    }

    mm_free(stack);
    mm_free(next);
}

/* Paper Figure 5, results stored ascending by node for each frontier. */
static void compute_frontier(DomTree *dt, Csr *preds)
{
    int n = dt->num_nodes;
    int *idom = dt->idom;
    int cap = 16;
    int num = 0;
    /* (runner, b) pairs */
    int *pairs = mm_alloc(sizeof(int) * 2 * cap);
    /* mark[runner] == b + 1 if b is already in DF(runner) */
    int *mark = mm_alloc(sizeof(int) * (n + 1));

    for (int b = 0; b < n; b++) {
        if (idom[b] < 0) continue;
        int stop = (b == dt->entry) ? -1 : idom[b];
        for (int k = preds->start[b]; k < preds->start[b + 1]; k++) {
            int runner = preds->list[k];
            if (idom[runner] < 0) continue;
            while (runner != stop) {
                if (mark[runner] == b + 1) break;
                mark[runner] = b + 1;
                if (num == cap) {
                    int *tmp = mm_alloc(sizeof(int) * 4 * cap);
                    memcpy(tmp, pairs, sizeof(int) * 2 * num);
                    mm_free(pairs);
                    pairs = tmp;
                    cap *= 2;
                }
                pairs[2 * num] = runner;
                pairs[2 * num + 1] = b;
                num++;
                if (runner == dt->entry) break;
                runner = idom[runner];
            }
        }
    }

    /* pairs are generated in ascending `b`, a stable bucket keeps that */
    Csr csr;
    csr_build(&csr, n, pairs, num, 0);
    dt->df_start = csr.start;
    dt->df_list = csr.list;

    mm_free(pairs);
    mm_free(mark);
}

int dom_tree_build(DomTree *dt, int num_nodes, int entry, const int *edges, int num_edges)
{
    memset(dt, 0, sizeof(*dt));
    if (num_nodes <= 0 || entry < 0 || entry >= num_nodes || num_edges < 0) return -1;
    for (int i = 0; i < 2 * num_edges; i++) {
        if (edges[i] < 0 || edges[i] >= num_nodes) return -1;
    }

    dt->num_nodes = num_nodes;
    dt->entry = entry;
    dt->rpo = mm_alloc(sizeof(int) * num_nodes);
    dt->rpo_index = mm_alloc(sizeof(int) * num_nodes);
    dt->idom = mm_alloc(sizeof(int) * num_nodes);
    dt->depth = mm_alloc(sizeof(int) * num_nodes);
    dt->dfs_in = mm_alloc(sizeof(int) * num_nodes);
    dt->dfs_out = mm_alloc(sizeof(int) * num_nodes);

    Csr succs, preds;
    csr_build(&succs, num_nodes, edges, num_edges, 0);
    csr_build(&preds, num_nodes, edges, num_edges, 1);

    compute_rpo(dt, &succs);
    compute_idom(dt, &preds);
    compute_tree(dt);
    compute_frontier(dt, &preds);

    csr_fini(&succs);
    csr_fini(&preds);
    return 0;
}

void dom_tree_fini(DomTree *dt)
{
    mm_free(dt->rpo);
    mm_free(dt->rpo_index);
    mm_free(dt->idom);
    mm_free(dt->depth);
    mm_free(dt->dfs_in);
    mm_free(dt->dfs_out);
    mm_free(dt->child_start);
    mm_free(dt->child_list);
    mm_free(dt->df_start);
    mm_free(dt->df_list);
    memset(dt, 0, sizeof(*dt));
}

int dom_dominates(DomTree *dt, int a, int b)
{
    if (dt->idom[a] < 0 || dt->idom[b] < 0) return 0;
    return dt->dfs_in[a] <= dt->dfs_in[b] && dt->dfs_out[b] <= dt->dfs_out[a];
}

#ifdef __cplusplus
}
#endif
