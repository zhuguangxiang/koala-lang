/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Optimistic value numbering, the reverse post-order (RPO) algorithm, after
 *
 *   L. Taylor Simpson,
 *   "Value-Driven Redundancy Elimination",
 *   PhD thesis, Rice University, 1996.
 *
 *   Preston Briggs, Keith D. Cooper, L. Taylor Simpson,
 *   "Value Numbering",
 *   Software Practice & Experience, 27(6):701-724, June 1997.
 *   https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/value-numbering.pdf
 *
 * Every value starts at TOP, "congruent to everything", and the whole
 * function is renumbered in reverse post-order until no value number
 * changes. Each pass uses a fresh hash table, so an assumption that turns out
 * to be false in one pass is dropped in the next:
 *
 *   for all SSA names i
 *       VN[i] <- TOP
 *   do
 *       changed <- false
 *       remove all entries from the hash table
 *       for each block b, in reverse postorder
 *           for each assignment a of the form "x <- y op z" in b
 *               expr <- <VN[y] op VN[z]>, simplified
 *               if expr is in the hash table with value number v
 *                   temp <- v
 *               else
 *                   temp <- x
 *                   add expr to the hash table with value number x
 *               if VN[x] != temp
 *                   changed <- true
 *                   VN[x] <- temp
 *   while changed
 *
 * A phi-function ignores its TOP inputs: on the first pass an input flowing
 * around a back edge is still TOP, so "i <- phi(0, i + 1)" is optimistically
 * assumed to be 0 and only gets its own value number when a later pass shows
 * that "i + 1" is not 0. Starting from optimism is what lets this algorithm
 * prove, for example, that two induction variables incremented in lockstep
 * are equal, which the pessimistic DVNT (gvn_dvnt.c) cannot.
 *
 * Reverse post-order makes every non-phi argument numbered before its use
 * in the same pass, because a definition dominates its uses and dominators
 * come first in RPO. Only phi inputs along back edges carry over from the
 * previous pass. The number of passes is bounded by the loop connectedness
 * of the CFG plus a small constant, as in the dominance algorithm.
 *
 * Value numbers only state congruence: two values with the same number are
 * equal whenever both are evaluated, but neither has to dominate the other.
 * A second step eliminates redundancies: walk the dominator tree with a
 * scoped table from value number to the first instruction of that number,
 * and replace an instruction only by such a leader, which dominates it.
 *
 * Notes on this implementation:
 *   - Simplification is constant folding, as in gvn_dvnt.c.
 *   - A phi whose non-TOP inputs all have one value number is meaningless
 *     and takes that number; otherwise it is hashed with its block and all
 *     inputs (TOP included), which finds redundant phis of the same block.
 *   - Value numbers are instruction ids, the first instruction that entered
 *     an expression into the table in the current pass.
 */

#include "dominance.h"
#include "gvn.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOP -1

/* key layout: op, block (PHI) or 0, imm low, imm high, nargs, args... */
#define KEY_HEAD 5

typedef struct _Slot {
    /* offset of the key in the pool, -1 if empty */
    int key;
    /* value number */
    int value;
    uint32_t hash;
} Slot;

/* Hash table, open addressing with linear probing, cleared every pass. */
typedef struct _Table {
    Slot *slots;
    int mask;
    int *pool;
    int pool_size;
} Table;

typedef struct _Rpo {
    const GvnFunc *fn;
    DomTree dt;
    /* predecessor of the k-th in-edge of each block */
    int *pred_start;
    int *pred_list;
    int *vn;
    /* constant value of a value number, valid in the current pass */
    char *is_const;
    int64_t *cval;
    Table table;
} Rpo;

static uint32_t key_hash(const int *key, int len)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) {
        h ^= (uint32_t)key[i];
        h *= 16777619u;
    }
    return h;
}

static int key_len(const int *key) { return KEY_HEAD + key[4]; }

static void table_clear(Table *t)
{
    for (int i = 0; i <= t->mask; i++) t->slots[i].key = -1;
    t->pool_size = 0;
}

/* Find `key`, or insert it with `value`. Returns the value in the table. */
static int table_find_or_insert(Table *t, int key, int value)
{
    const int *k = t->pool + key;
    int len = key_len(k);
    uint32_t hash = key_hash(k, len);
    int i = (int)(hash & (uint32_t)t->mask);
    while (t->slots[i].key >= 0) {
        Slot *s = &t->slots[i];
        const int *o = t->pool + s->key;
        if (s->hash == hash && key_len(o) == len && !memcmp(o, k, sizeof(int) * len))
            return s->value;
        i = (i + 1) & t->mask;
    }
    t->slots[i].key = key;
    t->slots[i].hash = hash;
    t->slots[i].value = value;
    t->pool_size = key + len;
    return value;
}

/* Write the key of `i` at the end of the pool, return its offset. */
static int make_key(Rpo *r, int b, int i)
{
    const GvnInsn *insn = &r->fn->insns[i];
    int *key = r->table.pool + r->table.pool_size;
    int64_t imm = insn->imm;
    int op = insn->op;
    int nargs = 0;

    if (op == GVN_PHI) {
        for (int k = 0; k < insn->num_args; k++) {
            int p = r->pred_list[r->pred_start[b] + k];
            if (!dom_reachable(&r->dt, p)) continue;
            key[KEY_HEAD + nargs++] = r->vn[insn->args[k]];
        }
        imm = 0;
    } else if (gvn_is_binary(insn->op)) {
        int a = r->vn[insn->args[0]];
        int c = r->vn[insn->args[1]];
        /* a definition dominates its uses, so it is numbered in this pass */
        ASSERT(a != TOP && c != TOP);
        if (r->is_const[a] && r->is_const[c]) {
            op = GVN_CONST;
            imm = gvn_eval(insn->op, r->cval[a], r->cval[c]);
        } else {
            if (gvn_is_commutative(insn->op) && a > c) {
                int tmp = a;
                a = c;
                c = tmp;
            }
            key[KEY_HEAD] = a;
            key[KEY_HEAD + 1] = c;
            nargs = 2;
            imm = 0;
        }
    }

    key[0] = op;
    key[1] = (op == GVN_PHI) ? b : 0;
    key[2] = (int)(uint32_t)((uint64_t)imm & 0xffffffffu);
    key[3] = (int)(uint32_t)((uint64_t)imm >> 32);
    key[4] = nargs;
    return r->table.pool_size;
}

/* New value number of `i` in this pass. */
static int number_insn(Rpo *r, int b, int i)
{
    const GvnInsn *insn = &r->fn->insns[i];
    if (insn->op == GVN_OPAQUE) return i;

    int key = make_key(r, b, i);
    int *k = r->table.pool + key;

    if (insn->op == GVN_PHI) {
        /* meaningless: all non-TOP inputs agree */
        int v = TOP;
        int same = 1;
        for (int j = 0; j < k[4]; j++) {
            int a = k[KEY_HEAD + j];
            if (a == TOP) continue;
            if (v == TOP)
                v = a;
            else if (a != v)
                same = 0;
        }
        if (same && v != TOP) return v;
    }

    /* the class of `i` is (re)defined by this key in this pass */
    if (k[0] == GVN_CONST) {
        r->is_const[i] = 1;
        r->cval[i] = (int64_t)((uint64_t)(uint32_t)k[2] | ((uint64_t)(uint32_t)k[3] << 32));
    } else {
        r->is_const[i] = 0;
    }
    return table_find_or_insert(&r->table, key, i);
}

static void number_all(Rpo *r)
{
    const GvnFunc *fn = r->fn;
    int changed = 1;
    int passes = 0;
    while (changed) {
        changed = 0;
        passes++;
        /* the fixed point is reached long before this */
        ASSERT(passes <= fn->num_insns + 3);
        table_clear(&r->table);
        for (int j = 0; j < r->dt.num_reachable; j++) {
            int b = r->dt.rpo[j];
            for (int i = fn->block_start[b]; i < fn->block_start[b + 1]; i++) {
                int v = number_insn(r, b, i);
                if (r->vn[i] != v) {
                    r->vn[i] = v;
                    changed = 1;
                }
            }
        }
    }
}

/*
 * Elimination: walk the dominator tree, keeping for each value number the
 * leader in scope, i.e. the first instruction of that number on the path
 * from entry. `leader[vn]` is that instruction or -1, and the undo log
 * restores it when a subtree is left.
 */
typedef struct _Elim {
    int *leader;
    int *log;
    int num_log;
    int *replace;
    int num_replaced;
} Elim;

static void eliminate_block(Rpo *r, Elim *e, int b)
{
    const GvnFunc *fn = r->fn;
    for (int i = fn->block_start[b]; i < fn->block_start[b + 1]; i++) {
        int v = r->vn[i];
        if (e->leader[v] >= 0) {
            e->replace[i] = e->leader[v];
            e->num_replaced++;
        } else {
            e->leader[v] = i;
            e->log[e->num_log++] = v;
        }
    }
}

static int eliminate(Rpo *r, int *replace)
{
    DomTree *dt = &r->dt;
    int n = r->fn->num_insns;
    int nb = dt->num_nodes;
    Elim e = { 0 };
    e.leader = mm_alloc(sizeof(int) * (n + 1));
    e.log = mm_alloc(sizeof(int) * (n + 1));
    e.replace = replace;
    int *stack = mm_alloc(sizeof(int) * (nb + 1));
    int *next = mm_alloc(sizeof(int) * (nb + 1));
    int *mark = mm_alloc(sizeof(int) * (nb + 1));
    int sp = 0;

    for (int i = 0; i < n; i++) e.leader[i] = -1;

    stack[sp++] = dt->entry;
    next[dt->entry] = 0;
    mark[dt->entry] = 0;
    eliminate_block(r, &e, dt->entry);

    while (sp > 0) {
        int b = stack[sp - 1];
        if (next[b] < dom_num_children(dt, b)) {
            int c = dom_children(dt, b)[next[b]++];
            next[c] = 0;
            mark[c] = e.num_log;
            stack[sp++] = c;
            eliminate_block(r, &e, c);
        } else {
            while (e.num_log > mark[b]) e.leader[e.log[--e.num_log]] = -1;
            sp--;
        }
    }

    mm_free(e.leader);
    mm_free(e.log);
    mm_free(stack);
    mm_free(next);
    mm_free(mark);
    return e.num_replaced;
}

int gvn_rpo(const GvnFunc *fn, int *replace)
{
    Rpo r;
    memset(&r, 0, sizeof(r));
    r.fn = fn;

    int n = fn->num_insns;
    for (int i = 0; i < n; i++) replace[i] = i;
    if (dom_tree_build(&r.dt, fn->num_blocks, fn->entry, fn->edges, fn->num_edges)) return 0;

    int nb = fn->num_blocks;
    r.pred_start = mm_alloc(sizeof(int) * (nb + 1));
    r.pred_list = mm_alloc(sizeof(int) * (fn->num_edges + 1));
    for (int e = 0; e < fn->num_edges; e++) r.pred_start[fn->edges[2 * e + 1] + 1]++;
    for (int b = 0; b < nb; b++) r.pred_start[b + 1] += r.pred_start[b];
    int *pos = mm_alloc(sizeof(int) * (nb + 1));
    memcpy(pos, r.pred_start, sizeof(int) * nb);
    for (int e = 0; e < fn->num_edges; e++)
        r.pred_list[pos[fn->edges[2 * e + 1]]++] = fn->edges[2 * e];
    mm_free(pos);

    r.vn = mm_alloc(sizeof(int) * (n + 1));
    for (int i = 0; i < n; i++) r.vn[i] = TOP;
    r.is_const = mm_alloc(n + 1);
    r.cval = mm_alloc(sizeof(int64_t) * (n + 1));

    int cap = 16;
    int pool = 0;
    for (int i = 0; i < n; i++) pool += KEY_HEAD + fn->insns[i].num_args;
    while (cap < 2 * n) cap <<= 1;
    r.table.slots = mm_alloc(sizeof(Slot) * cap);
    r.table.mask = cap - 1;
    r.table.pool = mm_alloc(sizeof(int) * (pool + 1));

    number_all(&r);
    int num_replaced = eliminate(&r, replace);

    mm_free(r.table.slots);
    mm_free(r.table.pool);
    mm_free(r.vn);
    mm_free(r.is_const);
    mm_free(r.cval);
    mm_free(r.pred_start);
    mm_free(r.pred_list);
    dom_tree_fini(&r.dt);
    return num_replaced;
}

#ifdef __cplusplus
}
#endif
