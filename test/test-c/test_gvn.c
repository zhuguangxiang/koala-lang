/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dominance.h"
#include "gvn.h"
#include "testhelp.h"

#define MAX_BLOCKS 16
#define MAX_EDGES 48
#define MAX_INSNS 256
#define MAX_ARGS 1024

/* ---------- a tiny SSA program builder ---------- */

typedef struct _Prog {
    int num_blocks;
    int num_edges;
    int edges[2 * MAX_EDGES];
    int num_insns;
    GvnInsn insns[MAX_INSNS];
    int block[MAX_INSNS];
    int num_args;
    int args[MAX_ARGS];
    int block_start[MAX_BLOCKS + 1];
    /* branch condition of each block for the interpreter, -1 for none */
    int cond[MAX_BLOCKS];
    GvnFunc fn;
} Prog;

static void prog_init(Prog *p, int num_blocks)
{
    memset(p, 0, sizeof(*p));
    p->num_blocks = num_blocks;
    for (int b = 0; b < MAX_BLOCKS; b++) p->cond[b] = -1;
}

static void edge(Prog *p, int src, int dst)
{
    p->edges[2 * p->num_edges] = src;
    p->edges[2 * p->num_edges + 1] = dst;
    p->num_edges++;
}

/* Instructions must be added in block order. */
static int insn(Prog *p, int b, GvnOp op, int64_t imm, int nargs, int a0, int a1)
{
    int i = p->num_insns++;
    GvnInsn *x = &p->insns[i];
    x->op = op;
    x->imm = imm;
    x->num_args = nargs;
    x->args = p->args + p->num_args;
    p->num_args += nargs;
    if (nargs > 0) x->args[0] = a0;
    if (nargs > 1) x->args[1] = a1;
    p->block[i] = b;
    return i;
}

static int cst(Prog *p, int b, int64_t v) { return insn(p, b, GVN_CONST, v, 0, 0, 0); }
static int param(Prog *p, int b, int idx) { return insn(p, b, GVN_PARAM, idx, 0, 0, 0); }
static int opaque(Prog *p, int b) { return insn(p, b, GVN_OPAQUE, 0, 0, 0, 0); }
static int bin(Prog *p, int b, GvnOp op, int x, int y) { return insn(p, b, op, 0, 2, x, y); }
/* two-input phi; set its args with phi_arg if they are defined later */
static int phi2(Prog *p, int b, int x, int y) { return insn(p, b, GVN_PHI, 0, 2, x, y); }

static void phi_arg(Prog *p, int i, int k, int v) { p->insns[i].args[k] = v; }

static GvnFunc *prog_func(Prog *p)
{
    int b = 0;
    for (int i = 0; i <= p->num_insns; i++) {
        int ib = (i < p->num_insns) ? p->block[i] : p->num_blocks;
        while (b <= ib && b <= p->num_blocks) p->block_start[b++] = i;
    }
    p->fn.num_blocks = p->num_blocks;
    p->fn.entry = 0;
    p->fn.num_edges = p->num_edges;
    p->fn.edges = p->edges;
    p->fn.num_insns = p->num_insns;
    p->fn.insns = p->insns;
    p->fn.block_start = p->block_start;
    return &p->fn;
}

/* ---------- checks ---------- */

/* Structural validity of `replace`. */
static int check_structure(Prog *p, const int *replace)
{
    DomTree dt;
    dom_tree_build(&dt, p->num_blocks, 0, p->edges, p->num_edges);
    int ok = 1;
    for (int i = 0; i < p->num_insns; i++) {
        int r = replace[i];
        if (r == i) continue;
        int bi = p->block[i];
        int br = p->block[r];
        if (r < 0 || r >= p->num_insns) ok = 0;
        /* the leader is kept */
        else if (replace[r] != r)
            ok = 0;
        /* unreachable code and opaque values are left alone */
        else if (!dom_reachable(&dt, bi) || p->insns[i].op == GVN_OPAQUE)
            ok = 0;
        /* the leader dominates the replaced instruction */
        else if (br == bi ? r >= i : !dom_strictly_dominates(&dt, br, bi))
            ok = 0;
    }
    dom_tree_fini(&dt);
    return ok;
}

static int64_t opaque_value(int i, int count)
{
    uint64_t h = (uint64_t)(i + 1) * 0x9e3779b97f4a7c15ull ^ (uint64_t)(count + 1) * 0xbf58476d1ce4e5b9ull;
    return (int64_t)(h ^ (h >> 31));
}

/*
 * Run the original and the rewritten program in lockstep: the rewritten one
 * only executes kept instructions and reads `replace[arg]`. After each
 * instruction `i`, its original value must equal the current value of
 * `replace[i]`, so both take the same path.
 */
static int check_run(Prog *p, const int *replace, const int64_t *params, int max_steps)
{
    static int64_t env_o[MAX_INSNS], env_t[MAX_INSNS], tmp_o[MAX_INSNS], tmp_t[MAX_INSNS];
    int cnt_o[MAX_INSNS] = { 0 };
    int cnt_t[MAX_INSNS] = { 0 };
    int in_pos[MAX_EDGES];
    int seen[MAX_BLOCKS] = { 0 };
    const GvnInsn *insns = p->insns;

    memset(env_o, 0, sizeof(env_o));
    memset(env_t, 0, sizeof(env_t));
    for (int e = 0; e < p->num_edges; e++) in_pos[e] = seen[p->edges[2 * e + 1]]++;

    int b = 0;
    int from = -1;
    for (int step = 0; step < max_steps; step++) {
        int s = p->block_start[b];
        int end = p->block_start[b + 1];

        /* phis, in parallel */
        int i = s;
        for (; i < end && insns[i].op == GVN_PHI; i++) {
            if (from < 0) return 0;
            int a = insns[i].args[in_pos[from]];
            tmp_o[i] = env_o[a];
            if (replace[i] == i) tmp_t[i] = env_t[replace[a]];
        }
        for (int j = s; j < i; j++) {
            env_o[j] = tmp_o[j];
            if (replace[j] == j) env_t[j] = tmp_t[j];
        }
        for (int j = s; j < i; j++) {
            if (env_o[j] != env_t[replace[j]]) return 0;
        }

        for (; i < end; i++) {
            const GvnInsn *x = &insns[i];
            int keep = replace[i] == i;
            switch (x->op) {
                case GVN_CONST:
                    env_o[i] = x->imm;
                    if (keep) env_t[i] = x->imm;
                    break;
                case GVN_PARAM:
                    env_o[i] = params[x->imm];
                    if (keep) env_t[i] = params[x->imm];
                    break;
                case GVN_OPAQUE:
                    env_o[i] = opaque_value(i, cnt_o[i]++);
                    if (keep) env_t[i] = opaque_value(i, cnt_t[i]++);
                    break;
                default:
                    env_o[i] = gvn_eval(x->op, env_o[x->args[0]], env_o[x->args[1]]);
                    if (keep)
                        env_t[i] = gvn_eval(x->op, env_t[replace[x->args[0]]],
                                            env_t[replace[x->args[1]]]);
                    break;
            }
            if (env_o[i] != env_t[replace[i]]) return 0;
        }

        /* branch on the condition, both runs agree on it by now */
        int outs[MAX_EDGES];
        int nout = 0;
        for (int e = 0; e < p->num_edges; e++) {
            if (p->edges[2 * e] == b) outs[nout++] = e;
        }
        if (nout == 0) return 1;
        uint64_t c = p->cond[b] >= 0 ? (uint64_t)env_o[p->cond[b]] : 0;
        from = outs[c % (uint64_t)nout];
        b = p->edges[2 * from + 1];
    }
    return 1;
}

/* ---------- random SSA programs ---------- */

static unsigned int seed = 20011114;

static int rnd(int m)
{
    seed = seed * 1103515245u + 12345u;
    return (int)((seed >> 16) % (unsigned int)m);
}

static void random_prog(Prog *p)
{
    int nb = 1 + rnd(10);
    prog_init(p, nb);

    for (int b = 0; b < nb; b++) {
        int nout = rnd(4) ? 1 + rnd(2) : 0;
        for (int k = 0; k < nout && p->num_edges < MAX_EDGES; k++) {
            int dst = rnd(10) < 7 ? b + 1 + rnd(3) : rnd(nb);
            edge(p, b, dst % nb);
        }
    }

    DomTree dt;
    dom_tree_build(&dt, nb, 0, p->edges, p->num_edges);

    int indeg[MAX_BLOCKS] = { 0 };
    for (int e = 0; e < p->num_edges; e++) indeg[p->edges[2 * e + 1]]++;

    /* layout: phis first, then params in entry, then operations */
    int num_phis[MAX_BLOCKS], num_ops[MAX_BLOCKS];
    for (int b = 0; b < nb; b++) {
        num_phis[b] = (b != 0 && indeg[b] > 0) ? rnd(4) : 0;
        num_ops[b] = rnd(7);
    }
    int params[2];
    for (int b = 0; b < nb; b++) {
        for (int k = 0; k < num_phis[b]; k++) {
            int i = insn(p, b, GVN_PHI, 0, 0, 0, 0);
            p->insns[i].num_args = indeg[b];
            p->insns[i].args = p->args + p->num_args;
            p->num_args += indeg[b];
        }
        if (b == 0) {
            params[0] = param(p, 0, 0);
            params[1] = param(p, 0, 1);
        }
        for (int k = 0; k < num_ops[b]; k++) insn(p, b, GVN_CONST, 0, 0, 0, 0);
    }
    prog_func(p);

    /* operations, dominators first */
    int order[MAX_BLOCKS];
    int no = 0;
    for (int j = 0; j < dt.num_reachable; j++) order[no++] = dt.rpo[j];
    for (int b = 0; b < nb; b++) {
        if (!dom_reachable(&dt, b)) order[no++] = b;
    }

    int avail[MAX_INSNS];
    for (int j = 0; j < nb; j++) {
        int b = order[j];
        for (int i = p->block_start[b]; i < p->block_start[b + 1]; i++) {
            GvnInsn *x = &p->insns[i];
            if (x->op == GVN_PHI || x->op == GVN_PARAM) continue;

            int na = 0;
            for (int k = 0; k < p->num_insns; k++) {
                int bk = p->block[k];
                if (bk == b ? k < i : dom_strictly_dominates(&dt, bk, b)) avail[na++] = k;
            }

            int r = rnd(100);
            if (r < 15 || na == 0) {
                x->op = GVN_CONST;
                x->imm = rnd(4);
            } else if (r < 20) {
                x->op = GVN_OPAQUE;
            } else {
                x->num_args = 2;
                x->args = p->args + p->num_args;
                p->num_args += 2;
                /* copy an earlier operation to create redundancy */
                int src = -1;
                if (rnd(100) < 40) {
                    int c = avail[rnd(na)];
                    if (gvn_is_binary(p->insns[c].op)) src = c;
                }
                if (src >= 0) {
                    x->op = p->insns[src].op;
                    int swap = gvn_is_commutative(x->op) && rnd(2);
                    x->args[0] = p->insns[src].args[swap];
                    x->args[1] = p->insns[src].args[!swap];
                } else {
                    x->op = (GvnOp)(GVN_ADD + rnd(GVN_LT - GVN_ADD + 1));
                    x->args[0] = avail[rnd(na)];
                    x->args[1] = avail[rnd(na)];
                }
            }
        }
    }

    /* phi inputs: anything dominating the end of the predecessor */
    for (int b = 0; b < nb; b++) {
        int first_phi = p->block_start[b];
        int k = 0;
        for (int e = 0; e < p->num_edges; e++) {
            if (p->edges[2 * e + 1] != b) continue;
            int pred = p->edges[2 * e];
            int na = 0;
            for (int c = 0; c < p->num_insns; c++) {
                if (dom_dominates(&dt, p->block[c], pred)) avail[na++] = c;
            }
            for (int i = first_phi; i < first_phi + num_phis[b]; i++) {
                int *a = p->insns[i].args;
                if (na == 0) {
                    /* unreachable predecessor, never taken */
                    a[k] = params[rnd(2)];
                } else if (i > first_phi && rnd(100) < 25) {
                    /* same input as the previous phi, may be redundant */
                    a[k] = p->insns[i - 1].args[k];
                } else if (k > 0 && rnd(100) < 40 && dom_dominates(&dt, p->block[a[k - 1]], pred) &&
                           dom_reachable(&dt, pred)) {
                    /* same input as the previous edge, may be meaningless */
                    a[k] = a[k - 1];
                } else {
                    a[k] = avail[rnd(na)];
                }
            }
            k++;
        }
    }

    /* branch conditions */
    for (int b = 0; b < nb; b++) {
        int na = 0;
        for (int c = 0; c < p->num_insns; c++) {
            if (dom_dominates(&dt, p->block[c], b)) avail[na++] = c;
        }
        p->cond[b] = na ? avail[rnd(na)] : -1;
    }

    dom_tree_fini(&dt);
}

/* ---------- tests ---------- */

static int replace_dvnt[MAX_INSNS];
static int replace_rpo[MAX_INSNS];

static void run_both(Prog *p)
{
    GvnFunc *fn = prog_func(p);
    gvn_dvnt(fn, replace_dvnt);
    gvn_rpo(fn, replace_rpo);
}

int gvnTest(void)
{
    static Prog p;

    /* ---------- one block: commutativity and folding ---------- */
    {
        prog_init(&p, 1);
        int a = param(&p, 0, 0);
        int b = param(&p, 0, 1);
        int x = bin(&p, 0, GVN_ADD, a, b);
        int y = bin(&p, 0, GVN_ADD, b, a);
        int s1 = bin(&p, 0, GVN_SUB, a, b);
        int s2 = bin(&p, 0, GVN_SUB, b, a);
        int c2 = cst(&p, 0, 2);
        int c3 = cst(&p, 0, 3);
        int f = bin(&p, 0, GVN_MUL, c2, c3);
        int c6 = cst(&p, 0, 6);
        int a2 = param(&p, 0, 0);
        int o1 = opaque(&p, 0);
        int o2 = opaque(&p, 0);
        run_both(&p);
        test_cond("DVNT: a+b == b+a", replace_dvnt[y] == x);
        test_cond("RPO: a+b == b+a", replace_rpo[y] == x);
        test_cond("DVNT/RPO: a-b != b-a", replace_dvnt[s2] == s2 && replace_rpo[s2] == s2 && s1 >= 0);
        test_cond("DVNT/RPO: 2*3 folds to the constant 6",
                  replace_dvnt[c6] == f && replace_rpo[c6] == f);
        test_cond("DVNT/RPO: same parameter twice",
                  replace_dvnt[a2] == a && replace_rpo[a2] == a);
        test_cond("DVNT/RPO: opaque values are never merged",
                  replace_dvnt[o2] == o2 && replace_rpo[o2] == o2 && o1 >= 0);
    }

    /* ---------- diamond: scope follows the dominator tree ----------
       0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3 */
    {
        prog_init(&p, 4);
        edge(&p, 0, 1);
        edge(&p, 0, 2);
        edge(&p, 1, 3);
        edge(&p, 2, 3);
        int a = param(&p, 0, 0);
        int b = param(&p, 0, 1);
        int x = bin(&p, 0, GVN_ADD, a, b);
        int y = bin(&p, 1, GVN_ADD, a, b);
        int m1 = bin(&p, 1, GVN_MUL, a, b);
        int m2 = bin(&p, 2, GVN_MUL, a, b);
        int ph = phi2(&p, 3, x, y);
        int p1 = phi2(&p, 3, m1, m2);
        int p2 = phi2(&p, 3, m1, m2);
        int m3 = bin(&p, 3, GVN_MUL, a, b);
        int w = bin(&p, 3, GVN_ADD, b, a);
        run_both(&p);
        test_cond("DVNT/RPO: dominating a+b is reused in a branch",
                  replace_dvnt[y] == x && replace_rpo[y] == x);
        test_cond("DVNT/RPO: a*b in a sibling branch is not reused",
                  replace_dvnt[m2] == m2 && replace_rpo[m2] == m2);
        test_cond("DVNT/RPO: a*b at the join is not replaced by a branch",
                  replace_dvnt[m3] == m3 && replace_rpo[m3] != m1 && replace_rpo[m3] != m2);
        test_cond("DVNT/RPO: meaningless phi(x, x) is x",
                  replace_dvnt[ph] == x && replace_rpo[ph] == x);
        test_cond("DVNT/RPO: redundant phi in the same block",
                  replace_dvnt[p2] == p1 && replace_rpo[p2] == p1);
        test_cond("DVNT/RPO: a+b at the join reuses entry",
                  replace_dvnt[w] == x && replace_rpo[w] == x);
        test_cond("Diamond: structure", check_structure(&p, replace_dvnt) &&
                                            check_structure(&p, replace_rpo));
    }

    /* ---------- loop: lockstep induction variables ----------
       0 -> 1, 2 -> 1, 1 -> 3 (exit), 1 -> 2 (taken while i < a)
       1: i = phi(0, i1); j = phi(0, j1); k = phi(a, k)
       2: i1 = i + 1; j1 = j + 1 */
    {
        prog_init(&p, 4);
        edge(&p, 0, 1);
        edge(&p, 2, 1);
        edge(&p, 1, 3);
        edge(&p, 1, 2);
        int a = param(&p, 0, 0);
        int c0 = cst(&p, 0, 0);
        int c1 = cst(&p, 0, 1);
        int i = phi2(&p, 1, c0, 0);
        int j = phi2(&p, 1, c0, 0);
        int k = phi2(&p, 1, a, 0);
        int lt = bin(&p, 1, GVN_LT, i, a);
        int i1 = bin(&p, 2, GVN_ADD, i, c1);
        int j1 = bin(&p, 2, GVN_ADD, c1, j);
        int ka = bin(&p, 2, GVN_ADD, k, c1);
        int aa = bin(&p, 2, GVN_ADD, a, c1);
        phi_arg(&p, i, 1, i1);
        phi_arg(&p, j, 1, j1);
        phi_arg(&p, k, 1, k);
        p.cond[1] = lt;
        run_both(&p);
        test_cond("DVNT: lockstep j is not proven equal to i", replace_dvnt[j] == j);
        test_cond("RPO: lockstep j == i and j+1 == i+1",
                  replace_rpo[j] == i && replace_rpo[j1] == i1);
        test_cond("DVNT: k = phi(a, k) is kept", replace_dvnt[k] == k);
        test_cond("RPO: k = phi(a, k) is a, and k+1 == a+1",
                  replace_rpo[k] == a && replace_rpo[aa] == ka);
        test_cond("Loop: structure", check_structure(&p, replace_dvnt) &&
                                         check_structure(&p, replace_rpo));
        int64_t args[2] = { 5, 0 };
        test_cond("Loop: rewritten programs compute the same values",
                  check_run(&p, replace_dvnt, args, 100) && check_run(&p, replace_rpo, args, 100));
    }

    /* ---------- loop: optimism has to be retracted ----------
       i = phi(0, i1), j = phi(0, j1); i1 = i + 1; j1 = j + 2 */
    {
        prog_init(&p, 4);
        edge(&p, 0, 1);
        edge(&p, 2, 1);
        edge(&p, 1, 3);
        edge(&p, 1, 2);
        int a = param(&p, 0, 0);
        int c0 = cst(&p, 0, 0);
        int c1 = cst(&p, 0, 1);
        int c2 = cst(&p, 0, 2);
        int i = phi2(&p, 1, c0, 0);
        int j = phi2(&p, 1, c0, 0);
        int lt = bin(&p, 1, GVN_LT, i, a);
        int i1 = bin(&p, 2, GVN_ADD, i, c1);
        int j1 = bin(&p, 2, GVN_ADD, j, c2);
        phi_arg(&p, i, 1, i1);
        phi_arg(&p, j, 1, j1);
        p.cond[1] = lt;
        run_both(&p);
        test_cond("RPO: i += 1 and j += 2 are different",
                  replace_rpo[j] == j && replace_rpo[i] == i && replace_rpo[j1] == j1);
        int64_t args[2] = { 5, 0 };
        test_cond("Retracted optimism: same values",
                  check_run(&p, replace_rpo, args, 100) && check_run(&p, replace_dvnt, args, 100));
    }

    /* ---------- unreachable block ---------- */
    {
        prog_init(&p, 3);
        edge(&p, 0, 2);
        edge(&p, 1, 2);
        int a = param(&p, 0, 0);
        int u = bin(&p, 1, GVN_ADD, a, a);
        int ph = phi2(&p, 2, a, u);
        run_both(&p);
        test_cond("DVNT/RPO: unreachable code is kept",
                  replace_dvnt[u] == u && replace_rpo[u] == u);
        test_cond("DVNT/RPO: edges from unreachable blocks are ignored",
                  replace_dvnt[ph] == a && replace_rpo[ph] == a);
    }

    /* ---------- random programs ---------- */
    {
        int ok_struct = 1, ok_run = 1, ok_stronger = 1;
        int total_dvnt = 0, total_rpo = 0;
        int failed = -1;
        for (int t = 0; t < 5000; t++) {
            random_prog(&p);
            GvnFunc *fn = prog_func(&p);
            int nd = gvn_dvnt(fn, replace_dvnt);
            int nr = gvn_rpo(fn, replace_rpo);
            total_dvnt += nd;
            total_rpo += nr;
            int ok = 1;
            if (!check_structure(&p, replace_dvnt) || !check_structure(&p, replace_rpo)) {
                ok_struct = 0;
                ok = 0;
            }
            for (int r = 0; r < 4; r++) {
                int64_t args[2] = { rnd(4) - 1, rnd(4) - 1 };
                if (r == 3) args[1] = INT64_MAX;
                if (!check_run(&p, replace_dvnt, args, 200) ||
                    !check_run(&p, replace_rpo, args, 200)) {
                    ok_run = 0;
                    ok = 0;
                }
            }
            /* RPO is optimistic, it finds at least what DVNT finds */
            if (nr < nd) {
                ok_stronger = 0;
                ok = 0;
            }
            if (!ok && failed < 0) failed = t;
        }
        if (failed >= 0) printf("random program #%d failed\n", failed);
        printf("random programs: DVNT replaced %d, RPO replaced %d\n", total_dvnt, total_rpo);
        test_cond("5000 random programs: replacements dominate their uses", ok_struct);
        test_cond("5000 random programs: rewritten programs compute the same values", ok_run);
        test_cond("5000 random programs: RPO replaces at least as much as DVNT", ok_stronger);
        test_cond("Random programs exercise both algorithms", total_dvnt > 0 && total_rpo > total_dvnt);
    }

    test_report();
    return 0;
}

int main(void) { return gvnTest(); }
