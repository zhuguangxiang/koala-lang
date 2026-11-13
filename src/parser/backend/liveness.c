/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Live ranges for the linear scan register allocator.
 *
 * A backward dataflow computes the values live into and out of each block, then each block is
 * walked backward to cut the life of every value into ranges. A value is not live in the gaps
 * between its ranges, e.g. in the other arm of an `if`, so its register may be given to
 * another value there.
 *
 * Positions follow klr_assign_coord: params are defined at 0 and every insn has its own pos.
 * An insn reads its operands and writes its result at its pos, a range is half-open, so a value
 * read last at p ends at p and the result of the insn at p may take its register.
 *
 *  - a `move %x, %v` writes %x, a write kills the old value of %x;
 *  - a `local` insn defines nothing, but nothing of %x is live above it;
 *  - a write that is never read still needs the register at its pos: [p, p + 1);
 *  - fixedslot insns are not tracked, cgen gives them call slots.
 */

#include "liveness.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t Word;

#define WORD_BITS     64
#define set_bit(s, i) ((s)[(i) / WORD_BITS] |= (Word)1 << ((i) % WORD_BITS))
#define has_bit(s, i) (((s)[(i) / WORD_BITS] >> ((i) % WORD_BITS)) & 1)

/* Insns that always write a result register, even when the result is unused: an operator on a
 * user class is kept by dce for its side effects, a 'new' is kept, e.g. its uses are inlined
 * away. A call with an unused result needs none (vreg -1 means no return register). */
static int unused_needs_reg(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_NEW:
        case OP_NUM_ADD:
        case OP_NUM_SUB:
        case OP_NUM_MUL:
        case OP_NUM_DIV:
        case OP_NUM_MOD:
        case OP_NUM_AND:
        case OP_NUM_OR:
        case OP_NUM_XOR:
        case OP_NUM_SHL:
        case OP_NUM_SHR:
        case OP_NUM_EQ:
        case OP_NUM_NE:
        case OP_NUM_LT:
        case OP_NUM_LE:
        case OP_NUM_GT:
        case OP_NUM_GE:
            return 1;
        default:
            return 0;
    }
}

static void range_reserve(KlrInterval *intv, int n)
{
    if (n <= intv->cap) return;
    int cap = intv->cap ? intv->cap * 2 : 4;
    while (cap < n) cap *= 2;
    KlrRange *ranges = mm_alloc(sizeof(KlrRange) * cap);
    if (intv->nranges) memcpy(ranges, intv->ranges, sizeof(KlrRange) * intv->nranges);
    mm_free(intv->ranges);
    intv->ranges = ranges;
    intv->cap = cap;
}

static void interval_update_bounds(KlrInterval *intv)
{
    ASSERT(intv->nranges > 0);
    intv->start = intv->ranges[0].start;
    intv->end = intv->ranges[intv->nranges - 1].end;
}

/* merge two sorted range lists into `dst`, joining overlapping and adjacent ranges */
static void ranges_merge(KlrInterval *dst, KlrRange *b, int nb)
{
    KlrRange *a = dst->ranges;
    int na = dst->nranges;
    KlrRange *out = mm_alloc(sizeof(KlrRange) * (na + nb));
    int n = 0, i = 0, j = 0;
    while (i < na || j < nb) {
        KlrRange r;
        if (j >= nb || (i < na && a[i].start <= b[j].start)) r = a[i++];
        else r = b[j++];
        if (n > 0 && r.start <= out[n - 1].end) {
            if (r.end > out[n - 1].end) out[n - 1].end = r.end;
        } else {
            out[n++] = r;
        }
    }
    mm_free(dst->ranges);
    dst->ranges = out;
    dst->nranges = n;
    dst->cap = na + nb;
    interval_update_bounds(dst);
}

void klr_interval_union(KlrInterval *dst, KlrInterval *src)
{
    if (src->nranges == 0) return;
    ranges_merge(dst, src->ranges, src->nranges);
}

void klr_interval_add(KlrInterval *intv, int start, int end)
{
    ASSERT(start < end);
    KlrRange r = { start, end };
    ranges_merge(intv, &r, 1);
}

int klr_interval_intersects(KlrInterval *a, KlrInterval *b)
{
    int i = a->cursor, j = b->cursor;
    while (i < a->nranges && j < b->nranges) {
        KlrRange *ra = &a->ranges[i];
        KlrRange *rb = &b->ranges[j];
        if (ra->start < rb->end && rb->start < ra->end) return 1;
        if (ra->end <= rb->end) i++;
        else j++;
    }
    return 0;
}

void klr_interval_fini(KlrInterval *intv)
{
    mm_free(intv->ranges);
    intv->ranges = NULL;
    intv->nranges = intv->cap = 0;
}

/* ranges arrive backward while a function is walked backward: prepend, kept reversed */
static void range_push_backward(KlrInterval *intv, int start, int end)
{
    ASSERT(start < end);
    if (intv->nranges > 0) {
        KlrRange *last = &intv->ranges[intv->nranges - 1];
        if (end >= last->start) {
            if (start < last->start) last->start = start;
            if (end > last->end) last->end = end;
            return;
        }
    }
    range_reserve(intv, intv->nranges + 1);
    intv->ranges[intv->nranges++] = (KlrRange){ start, end };
}

static int value_id(KlrLSRAContext *ctx, KlrValue *val)
{
    if (!val) return -1;
    if (!klr_is_param(val) && !(klr_is_insn(val) && !((KlrInsn *)val)->fixedslot)) return -1;
    int id = val->tag;
    if (id < 0 || id >= ctx->nids || ctx->vals[id] != val) return -1;
    return id;
}

KlrInterval *klr_value_interval(KlrLSRAContext *ctx, KlrValue *val)
{
    int id = value_id(ctx, val);
    if (id < 0 || ctx->imap[id] < 0) return NULL;
    return vector_get_ptr(&ctx->intervals, ctx->imap[id]);
}

/* the value an insn itself writes, or -1: a local is written by moves, not by its insn */
static int insn_def_id(KlrLSRAContext *ctx, KlrInsn *insn)
{
    if (insn->fixedslot || !ir_has_value(insn) || klr_is_local((KlrValue *)insn)) return -1;
    return value_id(ctx, (KlrValue *)insn);
}

typedef struct {
    KlrLSRAContext *ctx;
    KlrInterval *iv; /* per id, ranges kept reversed while building */
    int *end_at;     /* per id, end of the range open at the walk position, -1 if none */
    int *open;       /* ids with an open range */
    int nopen;
} RangeBuilder;

static void rb_use(RangeBuilder *rb, int id, int pos)
{
    if (rb->end_at[id] >= 0) return;
    rb->end_at[id] = pos;
    rb->open[rb->nopen++] = id;
}

/* close the open range of `id` at `pos`; a dead write still takes [pos, pos + 1) */
static void rb_def(RangeBuilder *rb, int id, int pos, int dead_needs_reg)
{
    int end = rb->end_at[id];
    if (end >= 0) {
        if (pos < end) range_push_backward(&rb->iv[id], pos, end);
        rb->end_at[id] = -1;
    } else if (dead_needs_reg) {
        range_push_backward(&rb->iv[id], pos, pos + 1);
    }
}

static void rb_flush(RangeBuilder *rb, int start, int entry)
{
    for (int k = 0; k < rb->nopen; k++) {
        int id = rb->open[k];
        int end = rb->end_at[id];
        if (end < 0) continue;
        /* a param is live from its definition at 0 */
        int s = (entry && klr_is_param(rb->ctx->vals[id])) ? 0 : start;
        if (s < end) range_push_backward(&rb->iv[id], s, end);
        rb->end_at[id] = -1;
    }
    rb->nopen = 0;
}

void klr_build_live_ranges(KlrLSRAContext *ctx)
{
    KlrFunc *fn = ctx->func;

    /* number the values */
    int nids = vector_size(&fn->params);
    int nblocks = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        nblocks++;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (!insn->fixedslot) nids++;
        }
    }

    ctx->nids = nids;
    ctx->vals = mm_alloc(sizeof(KlrValue *) * (nids + 1));
    ctx->imap = mm_alloc(sizeof(int) * (nids + 1));

    int id = 0;
    KlrParam *param;
    vector_foreach(param, &fn->params) {
        ctx->vals[id] = (KlrValue *)param;
        ((KlrValue *)param)->tag = id++;
    }

    KlrBasicBlock **blocks = mm_alloc(sizeof(KlrBasicBlock *) * (nblocks + 1));
    int tailcall_pos = 0;
    basic_block_foreach(bb, fn) {
        ASSERT(bb->index >= 0 && bb->index < nblocks);
        blocks[bb->index] = bb;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            // only a self tail call writes the params; another one passes call slots
            if (insn->code == OP_TAIL_CALL && insn_oper_value(insn, 0) == (KlrValue *)fn)
                tailcall_pos = MAX(tailcall_pos, insn->pos);
            if (insn->fixedslot) continue;
            ctx->vals[id] = (KlrValue *)insn;
            insn->tag = id++;
        }
    }

    /* gen: read before written in the block, kill: written in the block */
    int nw = (nids + WORD_BITS - 1) / WORD_BITS + 1;
    Word *sets = mm_alloc(sizeof(Word) * nw * nblocks * 4);
    memset(sets, 0, sizeof(Word) * nw * nblocks * 4);
#define GEN(i)      (sets + ((i) * 4 + 0) * nw)
#define KILL(i)     (sets + ((i) * 4 + 1) * nw)
#define LIVE_IN(i)  (sets + ((i) * 4 + 2) * nw)
#define LIVE_OUT(i) (sets + ((i) * 4 + 3) * nw)

    for (int b = 0; b < nblocks; b++) {
        Word *gen = GEN(b), *kill = KILL(b);
        KlrInsn *insn;
        insn_foreach(insn, blocks[b]) {
            for (int i = 0; i < insn->num_opers; i++) {
                KlrUse *use = &insn_operand(insn, i)->use;
                int vid = value_id(ctx, use->ref);
                if (vid < 0 || use->is_def) continue;
                if (!has_bit(kill, vid)) set_bit(gen, vid);
            }
            for (int i = 0; i < insn->num_opers; i++) {
                KlrUse *use = &insn_operand(insn, i)->use;
                int vid = value_id(ctx, use->ref);
                if (vid >= 0 && use->is_def) set_bit(kill, vid);
            }
            if (!insn->fixedslot && ir_has_value(insn)) {
                int vid = value_id(ctx, (KlrValue *)insn);
                if (vid >= 0) set_bit(kill, vid);
            }
        }
    }

    /* live_out = U live_in(succ), live_in = gen | (live_out & ~kill), to a fixed point */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int b = nblocks - 1; b >= 0; b--) {
            Word *out = LIVE_OUT(b), *in = LIVE_IN(b);
            KlrBasicBlock *succ;
            bb_succ_foreach(succ, blocks[b]) {
                int s = succ->index;
                if (s < 0 || s >= nblocks || blocks[s] != succ) continue;
                Word *sin = LIVE_IN(s);
                for (int w = 0; w < nw; w++) out[w] |= sin[w];
            }
            Word *gen = GEN(b), *kill = KILL(b);
            for (int w = 0; w < nw; w++) {
                Word v = gen[w] | (out[w] & ~kill[w]);
                if (v != in[w]) {
                    in[w] = v;
                    changed = 1;
                }
            }
        }
    }

    /* cut the ranges, walking the blocks and their insns backward */
    RangeBuilder rb;
    rb.ctx = ctx;
    rb.iv = mm_alloc(sizeof(KlrInterval) * (nids + 1));
    memset(rb.iv, 0, sizeof(KlrInterval) * (nids + 1));
    rb.end_at = mm_alloc(sizeof(int) * (nids + 1));
    for (int i = 0; i < nids; i++) rb.end_at[i] = -1;
    rb.open = mm_alloc(sizeof(int) * (nids + 1));
    rb.nopen = 0;

    for (int b = nblocks - 1; b >= 0; b--) {
        KlrBasicBlock *blk = blocks[b];
        Word *out = LIVE_OUT(b);
        for (int w = 0; w < nw; w++) {
            for (Word bits = out[w]; bits; bits &= bits - 1) {
                rb_use(&rb, w * WORD_BITS + __builtin_ctzll(bits), blk->last_pos);
            }
        }

        KlrInsn *insn;
        insn_foreach_reverse(insn, blk) {
            int pos = insn->pos;
            for (int i = 0; i < insn->num_opers; i++) {
                KlrUse *use = &insn_operand(insn, i)->use;
                int vid = value_id(ctx, use->ref);
                if (vid >= 0 && use->is_def) rb_def(&rb, vid, pos, 1);
            }

            if (klr_is_local((KlrValue *)insn) && !insn->fixedslot) {
                /* read before written, e.g. on another path: nothing is live above it */
                int vid = value_id(ctx, (KlrValue *)insn);
                if (vid >= 0) rb_def(&rb, vid, pos, 0);
            } else {
                int vid = insn_def_id(ctx, insn);
                if (vid >= 0) rb_def(&rb, vid, pos, unused_needs_reg(insn));
            }

            for (int i = 0; i < insn->num_opers; i++) {
                KlrUse *use = &insn_operand(insn, i)->use;
                int vid = value_id(ctx, use->ref);
                if (vid >= 0 && !use->is_def) rb_use(&rb, vid, pos);
            }
        }

        rb_flush(&rb, blk->first_pos, b == 0);
    }

#undef GEN
#undef KILL
#undef LIVE_IN
#undef LIVE_OUT

    /* the ranges are reversed */
    for (int vid = 0; vid < nids; vid++) {
        KlrInterval *iv = &rb.iv[vid];
        for (int i = 0, j = iv->nranges - 1; i < j; i++, j--) {
            KlrRange t = iv->ranges[i];
            iv->ranges[i] = iv->ranges[j];
            iv->ranges[j] = t;
        }
    }

    /* a param holds its slot from the entry; a self tail call writes the param slots (by moves
     * or by arguments computed straight into a slot), so no other value may take a param slot
     * before the last tail call */
    for (int vid = 0; vid < (int)vector_size(&fn->params); vid++) {
        KlrInterval *iv = &rb.iv[vid];
        if (iv->nranges == 0) range_push_backward(iv, 0, 1);
        else interval_update_bounds(iv);
        if (tailcall_pos > 0) klr_interval_add(iv, 0, tailcall_pos);
    }

    for (int vid = 0; vid < nids; vid++) {
        KlrInterval *iv = &rb.iv[vid];
        ctx->imap[vid] = -1;
        if (iv->nranges == 0) {
            log_info("no interval for %s, it is never read", klr_value_name(ctx->vals[vid]));
            continue;
        }
        interval_update_bounds(iv);
        iv->val = ctx->vals[vid];
        iv->cursor = 0;
        iv->allocated = 0;
        iv->hint = NULL;
        iv->alias = NULL;
        ctx->imap[vid] = vector_size(&ctx->intervals);
        vector_push_back(&ctx->intervals, iv);
    }

    mm_free(rb.iv);
    mm_free(rb.end_at);
    mm_free(rb.open);
    mm_free(sets);
    mm_free(blocks);
}

#ifdef __cplusplus
}
#endif
