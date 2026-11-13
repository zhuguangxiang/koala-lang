/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dominance.h"
#include "testhelp.h"

#define MAX_N 40

/* paper label `l` -> node index, labels are top..1 so the entry is node 0 */
#define P2N(top, l) ((top) - (l))

/* ---------- brute force, straight from the definitions ---------- */

/* Is `to` reachable from `entry` without passing `removed`? */
static int reach_without(int n, int entry, const int *edges, int ne, int removed, int to)
{
    char seen[MAX_N] = { 0 };
    int stack[MAX_N];
    int sp = 0;
    if (entry == removed) return 0;
    seen[entry] = 1;
    stack[sp++] = entry;
    while (sp > 0) {
        int u = stack[--sp];
        if (u == to) return 1;
        for (int i = 0; i < ne; i++) {
            int v = edges[2 * i + 1];
            if (edges[2 * i] == u && v != removed && !seen[v]) {
                seen[v] = 1;
                stack[sp++] = v;
            }
        }
    }
    return 0;
}

/* dom[a][b]: a dominates b, by "every path from entry to b passes a" */
static void brute_dom(int n, int entry, const int *edges, int ne, char dom[MAX_N][MAX_N])
{
    for (int b = 0; b < n; b++) {
        int reachable = reach_without(n, entry, edges, ne, -1, b);
        for (int a = 0; a < n; a++) {
            if (!reachable)
                dom[a][b] = 0;
            else if (a == b)
                dom[a][b] = 1;
            else
                dom[a][b] = !reach_without(n, entry, edges, ne, a, b);
        }
    }
}

/* Compare the tree with brute force; returns 1 if all agree. */
static int check_graph(int n, int entry, const int *edges, int ne)
{
    static char dom[MAX_N][MAX_N];
    brute_dom(n, entry, edges, ne, dom);

    DomTree dt;
    if (dom_tree_build(&dt, n, entry, edges, ne)) return 0;
    int ok = 1;

    for (int b = 0; b < n; b++) {
        int reachable = dom[b][b];
        if (dom_reachable(&dt, b) != reachable) ok = 0;

        /* idom: the strict dominator dominated by all other strict ones */
        int expect = reachable ? (b == entry ? entry : -1) : -1;
        if (reachable && b != entry) {
            for (int a = 0; a < n; a++) {
                if (a == b || !dom[a][b]) continue;
                int all = 1;
                for (int c = 0; c < n; c++) {
                    if (c != b && dom[c][b] && !dom[c][a]) all = 0;
                }
                if (all) expect = a;
            }
        }
        if (dom_idom(&dt, b) != expect) ok = 0;

        for (int a = 0; a < n; a++) {
            if (dom_dominates(&dt, a, b) != dom[a][b]) ok = 0;
        }

        /* DF(b) = { y : b dom some pred of y, and not b sdom y } */
        int k = 0;
        int nf = dom_num_frontier(&dt, b);
        int *df = dom_frontier(&dt, b);
        for (int y = 0; y < n; y++) {
            int in = 0;
            for (int i = 0; i < ne; i++) {
                if (edges[2 * i + 1] == y && dom[b][edges[2 * i]]) in = 1;
            }
            if (in && b != y && dom[b][y]) in = 0;
            if (in) {
                if (k >= nf || df[k] != y) ok = 0;
                k++;
            }
        }
        if (k != nf) ok = 0;

        /* tree shape: children have this node as idom, depth is consistent */
        for (int i = 0; i < dom_num_children(&dt, b); i++) {
            int c = dom_children(&dt, b)[i];
            if (dom_idom(&dt, c) != b || dt.depth[c] != dt.depth[b] + 1) ok = 0;
        }
    }

    /* rpo starts at entry and each reachable node appears once */
    if (dt.rpo[0] != entry) ok = 0;
    for (int i = 0; i < dt.num_reachable; i++) {
        if (dt.rpo_index[dt.rpo[i]] != i) ok = 0;
    }

    dom_tree_fini(&dt);
    return ok;
}

static unsigned int seed = 12345;

static int rnd(int m)
{
    seed = seed * 1103515245u + 12345u;
    return (int)((seed >> 16) % (unsigned int)m);
}

int dominanceTest(void)
{
    DomTree dt;

    /* ---------- CHK paper Figure 2 ----------
       Cooper, Harvey, Kennedy, "A Simple, Fast Dominance Algorithm", 2001,
       https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/dom14.pdf
       Irreducible graph, nodes labelled by postorder 5..1, entry 5:
         5 -> 3, 5 -> 4, 3 -> 2, 2 -> 1, 1 -> 2, 4 -> 1
       Edges are listed in the DFS order that yields the paper's postorder.
       Paper: Dom[2] is wrong ({5,3,2}) after the first pass, fixed in the
       second, and the third pass changes nothing. */
    {
        int lab[] = { 5, 3, 5, 4, 3, 2, 2, 1, 1, 2, 4, 1 };
        int e[12];
        for (int i = 0; i < 12; i++) e[i] = P2N(5, lab[i]);
        dom_tree_build(&dt, 5, P2N(5, 5), e, 6);
        int ok = 1;
        for (int i = 0; i < 5; i++) ok &= dt.rpo[i] == P2N(5, 5 - i);
        test_cond("CHK Figure 2: RPO is 5 4 3 2 1", ok);
        ok = 1;
        for (int l = 1; l <= 5; l++) ok &= dom_idom(&dt, P2N(5, l)) == P2N(5, 5);
        test_cond("CHK Figure 2: doms[b] = 5 for every b", ok);
        test_cond("CHK Figure 2: 3 iterations", dt.num_iterations == 3);
        test_cond("CHK Figure 2: DF(1) = {2}, DF(2) = {1}",
                  dom_num_frontier(&dt, P2N(5, 1)) == 1 &&
                      dom_frontier(&dt, P2N(5, 1))[0] == P2N(5, 2) &&
                      dom_num_frontier(&dt, P2N(5, 2)) == 1 &&
                      dom_frontier(&dt, P2N(5, 2))[0] == P2N(5, 1));
        dom_tree_fini(&dt);
        test_cond("CHK Figure 2 matches brute force", check_graph(5, P2N(5, 5), e, 6));
    }

    /* ---------- CHK paper Figure 4 ----------
       Same paper. Nodes labelled by postorder 6..1, entry 6:
         6 -> 4, 6 -> 5, 4 -> 3, 4 -> 2, 3 -> 2, 2 -> 1, 2 -> 3, 1 -> 2, 5 -> 1
       doms after each pass (paper table):
         pass 1: 5:6 4:6 3:4 2:4 1:6
         pass 2: 2:6
         pass 3: 3:6
         pass 4: no change, "four iterations instead of three" */
    {
        int lab[] = { 6, 4, 6, 5, 4, 3, 4, 2, 3, 2, 2, 1, 2, 3, 1, 2, 5, 1 };
        int e[18];
        for (int i = 0; i < 18; i++) e[i] = P2N(6, lab[i]);
        dom_tree_build(&dt, 6, P2N(6, 6), e, 9);
        int ok = 1;
        for (int i = 0; i < 6; i++) ok &= dt.rpo[i] == P2N(6, 6 - i);
        test_cond("CHK Figure 4: RPO is 6 5 4 3 2 1", ok);
        ok = 1;
        for (int l = 1; l <= 6; l++) ok &= dom_idom(&dt, P2N(6, l)) == P2N(6, 6);
        test_cond("CHK Figure 4: doms[b] = 6 for every b", ok);
        test_cond("CHK Figure 4: 4 iterations", dt.num_iterations == 4);
        test_cond("CHK Figure 4: entry has 5 children",
                  dom_num_children(&dt, P2N(6, 6)) == 5);
        /* Figure 5 on this graph */
        test_cond("CHK Figure 4: DF(2) = {1, 3}, DF(4) = {2, 3}",
                  dom_num_frontier(&dt, P2N(6, 2)) == 2 &&
                      dom_frontier(&dt, P2N(6, 2))[0] == P2N(6, 3) &&
                      dom_frontier(&dt, P2N(6, 2))[1] == P2N(6, 1) &&
                      dom_num_frontier(&dt, P2N(6, 4)) == 2 &&
                      dom_frontier(&dt, P2N(6, 4))[0] == P2N(6, 3) &&
                      dom_frontier(&dt, P2N(6, 4))[1] == P2N(6, 2));
        dom_tree_fini(&dt);
        test_cond("CHK Figure 4 matches brute force", check_graph(6, P2N(6, 6), e, 9));
    }

    /* ---------- Engineering a Compiler, chapter 9 ----------
       Keith D. Cooper, Linda Torczon, "Engineering a Compiler", 2nd ed.,
       Morgan Kaufmann, 2011, section 9.2.1 (dominance) and section 9.3.2
       (dominance frontiers); the same CFG B0..B8 is used for SSA:
         B0 -> B1, B1 -> B2, B1 -> B5, B2 -> B3, B5 -> B6, B5 -> B8,
         B6 -> B7, B8 -> B7, B7 -> B3, B3 -> B4, B3 -> B1 (back edge)
       Dom: B0{0} B1{0,1} B2{0,1,2} B3{0,1,3} B4{0,1,3,4}
            B5{0,1,5} B6{0,1,5,6} B7{0,1,5,7} B8{0,1,5,8}
       DF:  B1{B1} B2{B3} B3{B1} B5{B3} B6{B7} B7{B3} B8{B7}, others empty */
    {
        int e[] = { 0, 1, 1, 2, 1, 5, 2, 3, 5, 6, 5, 8, 6, 7, 8, 7, 7, 3, 3, 4, 3, 1 };
        dom_tree_build(&dt, 9, 0, e, 11);
        static const int idom[9] = { 0, 0, 1, 1, 3, 1, 5, 5, 5 };
        int ok = 1;
        for (int b = 0; b < 9; b++) ok &= dom_idom(&dt, b) == idom[b];
        test_cond("EaC B0..B8: IDom", ok);

        static const char *dom_sets[9] = { "0",   "01",  "012", "013", "0134",
                                           "015", "0156", "0157", "0158" };
        ok = 1;
        for (int b = 0; b < 9; b++) {
            for (int a = 0; a < 9; a++) {
                int in = 0;
                for (const char *c = dom_sets[b]; *c; c++) in |= (*c - '0') == a;
                ok &= dom_dominates(&dt, a, b) == in;
            }
        }
        test_cond("EaC B0..B8: Dom sets", ok);

        static const int df[9] = { -1, 1, 3, 1, -1, 3, 7, 3, 7 };
        ok = 1;
        for (int b = 0; b < 9; b++) {
            if (df[b] < 0)
                ok &= dom_num_frontier(&dt, b) == 0;
            else
                ok &= dom_num_frontier(&dt, b) == 1 && dom_frontier(&dt, b)[0] == df[b];
        }
        test_cond("EaC B0..B8: dominance frontiers", ok);
        test_cond("EaC B0..B8: B3 -> B1 is a back edge (B1 dom B3)",
                  dom_dominates(&dt, 1, 3) && !dom_dominates(&dt, 3, 1));
        test_cond("EaC B0..B8: reducible, 2 iterations", dt.num_iterations == 2);
        dom_tree_fini(&dt);
        test_cond("EaC B0..B8 matches brute force", check_graph(9, 0, e, 11));
    }

    /* ---------- if / else diamond ----------
       0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3 */
    {
        int e[] = { 0, 1, 0, 2, 1, 3, 2, 3 };
        dom_tree_build(&dt, 4, 0, e, 4);
        test_cond("Diamond: join is dominated by the branch",
                  dom_idom(&dt, 3) == 0 && dom_idom(&dt, 1) == 0 && dom_idom(&dt, 2) == 0);
        test_cond("Diamond: arms don't dominate the join",
                  !dom_dominates(&dt, 1, 3) && !dom_dominates(&dt, 2, 3));
        test_cond("Diamond: DF(then) = DF(else) = { join }",
                  dom_num_frontier(&dt, 1) == 1 && dom_frontier(&dt, 1)[0] == 3 &&
                      dom_num_frontier(&dt, 2) == 1 && dom_frontier(&dt, 2)[0] == 3);
        test_cond("Diamond: DF(entry) and DF(join) are empty",
                  dom_num_frontier(&dt, 0) == 0 && dom_num_frontier(&dt, 3) == 0);
        dom_tree_fini(&dt);
    }

    /* ---------- while loop ----------
       0 -> 1 (header), 1 -> 2 (body), 2 -> 1 (back edge), 1 -> 3 (exit) */
    {
        int e[] = { 0, 1, 1, 2, 2, 1, 1, 3 };
        dom_tree_build(&dt, 4, 0, e, 4);
        test_cond("Loop: header dominates body and exit",
                  dom_idom(&dt, 2) == 1 && dom_idom(&dt, 3) == 1);
        test_cond("Loop: back edge target dominates its source", dom_dominates(&dt, 1, 2));
        test_cond("Loop: DF(body) = { header }",
                  dom_num_frontier(&dt, 2) == 1 && dom_frontier(&dt, 2)[0] == 1);
        test_cond("Loop: header is in its own DF",
                  dom_num_frontier(&dt, 1) == 1 && dom_frontier(&dt, 1)[0] == 1);
        test_cond("Loop: depths", dt.depth[0] == 0 && dt.depth[1] == 1 && dt.depth[2] == 2);
        dom_tree_fini(&dt);
    }

    /* ---------- unreachable node, self loop, back edge into entry ---------- */
    {
        /* 0 -> 1, 1 -> 1, 1 -> 0, 2 -> 1 (2 is unreachable) */
        int e[] = { 0, 1, 1, 1, 1, 0, 2, 1 };
        dom_tree_build(&dt, 3, 0, e, 4);
        test_cond("Unreachable node has no idom",
                  !dom_reachable(&dt, 2) && dom_idom(&dt, 2) == -1 && dt.depth[2] == -1);
        test_cond("Unreachable node neither dominates nor is dominated",
                  !dom_dominates(&dt, 0, 2) && !dom_dominates(&dt, 2, 2) &&
                      !dom_dominates(&dt, 2, 1));
        test_cond("Unreachable predecessor is ignored", dom_idom(&dt, 1) == 0);
        test_cond("Entry with an incoming edge is in its own DF",
                  dom_num_frontier(&dt, 0) == 1 && dom_frontier(&dt, 0)[0] == 0);
        dom_tree_fini(&dt);
        test_cond("Unreachable/self loop graph matches brute force", check_graph(3, 0, e, 4));
    }

    /* ---------- single node, non-zero entry ---------- */
    {
        dom_tree_build(&dt, 1, 0, NULL, 0);
        test_cond("Single node dominates itself",
                  dom_dominates(&dt, 0, 0) && dom_idom(&dt, 0) == 0 && dt.num_reachable == 1);
        dom_tree_fini(&dt);

        int e[] = { 2, 0, 0, 1 };
        test_cond("Non-zero entry matches brute force", check_graph(3, 2, e, 2));
    }

    /* ---------- invalid arguments ---------- */
    {
        int e[] = { 0, 5 };
        test_cond("Out of range edge is rejected", dom_tree_build(&dt, 2, 0, e, 1) == -1);
        test_cond("Out of range entry is rejected", dom_tree_build(&dt, 2, 2, NULL, 0) == -1);
        test_cond("Empty graph is rejected", dom_tree_build(&dt, 0, 0, NULL, 0) == -1);
    }

    /* ---------- random graphs vs brute force ---------- */
    {
        static int e[2 * MAX_N * 4];
        int ok = 1;
        int failed = -1;
        for (int t = 0; t < 2000 && ok; t++) {
            int n = 1 + rnd(MAX_N);
            int ne = rnd(n * 3 + 1);
            for (int i = 0; i < ne; i++) {
                e[2 * i] = rnd(n);
                /* bias towards forward edges like a real CFG */
                e[2 * i + 1] = rnd(4) ? (e[2 * i] + 1 + rnd(3)) % n : rnd(n);
            }
            if (!check_graph(n, rnd(n), e, ne)) {
                ok = 0;
                failed = t;
            }
        }
        if (!ok) printf("random graph #%d failed\n", failed);
        test_cond("2000 random graphs match brute force", ok);
    }

    test_report();
    return 0;
}

int main(void) { return dominanceTest(); }
