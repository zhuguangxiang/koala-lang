/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Dominator-based Value Numbering Technique (DVNT), after
 *
 *   Preston Briggs, Keith D. Cooper, L. Taylor Simpson,
 *   "Value Numbering",
 *   Software Practice & Experience, 27(6):701-724, June 1997.
 *   https://www.cs.tufts.edu/comp/150FP/archive/keith-cooper/value-numbering.pdf
 *
 * Hash-based local value numbering is extended to the whole SSA function by
 * walking the dominator tree. Because of SSA, a name is defined once and
 * its definition dominates every use, so the value table built along the
 * path from entry to a block is valid in that block; scoped hash tables
 * discard the facts of a subtree when the walk leaves it. The paper's
 * pseudo code:
 *
 *   procedure DVNT(B)
 *       mark the beginning of a new scope
 *       for each phi-function p of the form "n <- phi(...)" in B
 *           if p is meaningless or redundant
 *               put the value number for p into VN[n]
 *               remove p
 *           else
 *               VN[n] <- n
 *               add p to the hash table
 *       for each assignment a of the form "x <- y op z" in B
 *           overwrite y with VN[y] and z with VN[z]
 *           expr <- <y op z>
 *           if expr can be simplified to expr'
 *               replace a with "x <- expr'"
 *               expr <- expr'
 *           if expr is found in the hash table with value number v
 *               VN[x] <- v
 *               remove a
 *           else
 *               VN[x] <- x
 *               add expr to the hash table with value number x
 *       for each successor s of B
 *           adjust the phi-function inputs in s
 *       for each child c of B in the dominator tree
 *           DVNT(c)
 *       clean up the hash table after leaving this scope
 *
 * A phi-function is meaningless when all its inputs have the same value
 * number, and redundant when it computes the same value as another
 * phi-function in the same block (same inputs, in the same order).
 *
 * Notes on this implementation:
 *   - "adjust the phi-function inputs in s" is done lazily: when a block is
 *     visited, an input takes VN[arg] if `arg` was already numbered and
 *     stays `arg` otherwise (an input flowing around a back edge). VN[x]
 *     starts as x, so both cases are just VN[arg]. Children are visited in
 *     reverse post-order (dominance.c), so every forward predecessor of a
 *     block is numbered before the block itself.
 *   - Simplification is constant folding: an operator on two constants
 *     hashes as the folded constant.
 *   - Phi keys include the block, PARAM keys the parameter index, OPAQUE
 *     values are never entered into the table.
 *
 * DVNT is pessimistic: a value flowing around a back edge is assumed to be
 * different from everything until proven otherwise, so it cannot discover
 * congruences carried around loops; see gvn_rpo.c for that.
 */

#include "dominance.h"
#include "gvn.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* key layout: op, block (PHI) or 0, imm low, imm high, nargs, args... */
#define KEY_HEAD 5

typedef struct _Slot {
    /* offset of the key in the pool, -1 if empty */
    int key;
    /* value number */
    int value;
    uint32_t hash;
} Slot;

/*
 * Scoped hash table, open addressing with linear probing. Entries are only
 * removed in exact reverse order of insertion, which restores the probe
 * sequences exactly, so no tombstones are needed.
 */
typedef struct _ScopedTable {
    Slot *slots;
    int mask;
    /* inserted slot indexes, the undo log */
    int *log;
    int num_log;
    /* key pool */
    int *pool;
    int pool_size;
} ScopedTable;

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

static int table_lookup(ScopedTable *t, const int *key, uint32_t hash, int *slot)
{
    int len = key_len(key);
    int i = (int)(hash & (uint32_t)t->mask);
    while (t->slots[i].key >= 0) {
        Slot *s = &t->slots[i];
        const int *k = t->pool + s->key;
        if (s->hash == hash && key_len(k) == len && !memcmp(k, key, sizeof(int) * len)) {
            *slot = i;
            return s->value;
        }
        i = (i + 1) & t->mask;
    }
    *slot = i;
    return -1;
}

/* `key` must be at the end of the pool, it is kept there. */
static void table_insert(ScopedTable *t, int slot, int key, uint32_t hash, int value)
{
    t->slots[slot].key = key;
    t->slots[slot].hash = hash;
    t->slots[slot].value = value;
    t->log[t->num_log++] = slot;
    t->pool_size = key + key_len(t->pool + key);
}

static void table_pop(ScopedTable *t, int mark)
{
    while (t->num_log > mark) {
        Slot *s = &t->slots[t->log[--t->num_log]];
        t->pool_size = s->key;
        s->key = -1;
    }
}

typedef struct _Dvnt {
    const GvnFunc *fn;
    DomTree dt;
    /* preds of each block, as edge-ordered argument positions */
    int *pred_start;
    int *pred_list;
    /* value number of each insn */
    int *vn;
    /* constant value of a value number */
    char *is_const;
    int64_t *cval;
    int *replace;
    int num_replaced;
    ScopedTable table;
} Dvnt;

/* Write the key of `i` at the end of the pool, return its offset. */
static int make_key(Dvnt *d, int b, int i)
{
    const GvnInsn *insn = &d->fn->insns[i];
    int *key = d->table.pool + d->table.pool_size;
    int64_t imm = insn->imm;
    int op = insn->op;
    int nargs = 0;

    if (op == GVN_PHI) {
        for (int k = 0; k < insn->num_args; k++) {
            int p = d->pred_list[d->pred_start[b] + k];
            /* edges from unreachable blocks are never taken */
            if (!dom_reachable(&d->dt, p)) continue;
            key[KEY_HEAD + nargs++] = d->vn[insn->args[k]];
        }
        imm = 0;
    } else if (gvn_is_binary(insn->op)) {
        int a = d->vn[insn->args[0]];
        int c = d->vn[insn->args[1]];
        if (d->is_const[a] && d->is_const[c]) {
            op = GVN_CONST;
            imm = gvn_eval(insn->op, d->cval[a], d->cval[c]);
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
    return d->table.pool_size;
}

/* VN[i] <- v, and remove i if v is another value. */
static void set_vn(Dvnt *d, int i, int v)
{
    d->vn[i] = v;
    if (v != i) {
        d->replace[i] = v;
        d->num_replaced++;
    }
}

static void number_insn(Dvnt *d, int b, int i)
{
    const GvnInsn *insn = &d->fn->insns[i];
    if (insn->op == GVN_OPAQUE) return;

    int key = make_key(d, b, i);
    int *k = d->table.pool + key;

    /* meaningless phi: all inputs have the same value number */
    if (insn->op == GVN_PHI && k[4] > 0) {
        int same = 1;
        for (int j = 1; j < k[4]; j++) same &= k[KEY_HEAD + j] == k[KEY_HEAD];
        if (same) {
            set_vn(d, i, k[KEY_HEAD]);
            return;
        }
    }

    uint32_t hash = key_hash(k, key_len(k));
    int slot;
    int v = table_lookup(&d->table, k, hash, &slot);
    if (v >= 0) {
        set_vn(d, i, v);
        return;
    }

    table_insert(&d->table, slot, key, hash, i);
    if (k[0] == GVN_CONST) {
        d->is_const[i] = 1;
        d->cval[i] = (int64_t)((uint64_t)(uint32_t)k[2] | ((uint64_t)(uint32_t)k[3] << 32));
    }
}

/* Iterative preorder walk of the dominator tree with scopes. */
static void walk(Dvnt *d)
{
    DomTree *dt = &d->dt;
    int n = dt->num_nodes;
    int *stack = mm_alloc(sizeof(int) * (n + 1));
    int *next = mm_alloc(sizeof(int) * (n + 1));
    int *mark = mm_alloc(sizeof(int) * (n + 1));
    int sp = 0;

    stack[sp++] = dt->entry;
    next[dt->entry] = 0;
    mark[dt->entry] = d->table.num_log;
    for (int i = d->fn->block_start[dt->entry]; i < d->fn->block_start[dt->entry + 1]; i++)
        number_insn(d, dt->entry, i);

    while (sp > 0) {
        int b = stack[sp - 1];
        if (next[b] < dom_num_children(dt, b)) {
            int c = dom_children(dt, b)[next[b]++];
            next[c] = 0;
            mark[c] = d->table.num_log;
            stack[sp++] = c;
            for (int i = d->fn->block_start[c]; i < d->fn->block_start[c + 1]; i++)
                number_insn(d, c, i);
        } else {
            table_pop(&d->table, mark[b]);
            sp--;
        }
    }

    mm_free(stack);
    mm_free(next);
    mm_free(mark);
}

int gvn_dvnt(const GvnFunc *fn, int *replace)
{
    Dvnt d;
    memset(&d, 0, sizeof(d));
    d.fn = fn;
    d.replace = replace;

    int n = fn->num_insns;
    for (int i = 0; i < n; i++) replace[i] = i;
    if (dom_tree_build(&d.dt, fn->num_blocks, fn->entry, fn->edges, fn->num_edges)) return 0;

    /* predecessor of the k-th in-edge of each block */
    int nb = fn->num_blocks;
    d.pred_start = mm_alloc(sizeof(int) * (nb + 1));
    d.pred_list = mm_alloc(sizeof(int) * (fn->num_edges + 1));
    for (int e = 0; e < fn->num_edges; e++) d.pred_start[fn->edges[2 * e + 1] + 1]++;
    for (int b = 0; b < nb; b++) d.pred_start[b + 1] += d.pred_start[b];
    int *pos = mm_alloc(sizeof(int) * (nb + 1));
    memcpy(pos, d.pred_start, sizeof(int) * nb);
    for (int e = 0; e < fn->num_edges; e++)
        d.pred_list[pos[fn->edges[2 * e + 1]]++] = fn->edges[2 * e];
    mm_free(pos);

    d.vn = mm_alloc(sizeof(int) * (n + 1));
    for (int i = 0; i < n; i++) d.vn[i] = i;
    d.is_const = mm_alloc(n + 1);
    d.cval = mm_alloc(sizeof(int64_t) * (n + 1));

    int cap = 16;
    int pool = 0;
    for (int i = 0; i < n; i++) pool += KEY_HEAD + fn->insns[i].num_args;
    while (cap < 2 * n) cap <<= 1;
    d.table.slots = mm_alloc(sizeof(Slot) * cap);
    for (int i = 0; i < cap; i++) d.table.slots[i].key = -1;
    d.table.mask = cap - 1;
    d.table.log = mm_alloc(sizeof(int) * (n + 1));
    d.table.pool = mm_alloc(sizeof(int) * (pool + 1));

    walk(&d);

    mm_free(d.table.slots);
    mm_free(d.table.log);
    mm_free(d.table.pool);
    mm_free(d.vn);
    mm_free(d.is_const);
    mm_free(d.cval);
    mm_free(d.pred_start);
    mm_free(d.pred_list);
    dom_tree_fini(&d.dt);
    return d.num_replaced;
}

#ifdef __cplusplus
}
#endif
