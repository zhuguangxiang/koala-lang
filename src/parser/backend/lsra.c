/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "lsra.h"
#include "bitset.h"
#include "liveness.h"
#include "cmd.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void klr_lsra_dump(KlrLSRAContext *ctx)
{
    KlrFunc *fn = ctx->func;

    update_tags(fn);

    KlrKlass *kls = fn->klass;
    if (kls) {
        fprintf(stdout, "\n=============== LSRA @%s::%s ===============\n", kls->name, fn->name);
    } else {
        fprintf(stdout, "\n=============== LSRA @%s ===============\n", fn->name);
    }

    fprintf(stdout, "\n--- Intervals ---\n");

    KlrInterval *intv;
    vector_foreach_ptr(intv, &ctx->intervals) {
        KlrValue *val = intv->val;

        fprintf(stdout, "\n%s: ", klr_value_name(val));
        fprintf(stdout, "range: ");
        for (int i = 0; i < intv->nranges; i++) {
            fprintf(stdout, "[%d, %d) ", intv->ranges[i].start, intv->ranges[i].end);
        }
        fprintf(stdout, "reg: %d", val->vreg);
        ASSERT(intv->start < intv->end);
        ASSERT(val->vreg >= 0);
    }

    fprintf(stdout, "\n\n--- Timeline & Registers ---\n\n");

    KlrParam *param;
    vector_foreach(param, &fn->params) {
        KlrValue *val = (KlrValue *)param;
        fprintf(stdout, "   0: [R%d] param %s\n", val->vreg, klr_value_name(val));
    }

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        fprintf(stdout, "\n%%%s[idx:%d,pos:%d-%d]:\n", klr_block_name(bb), bb->index, bb->first_pos,
                bb->last_pos - 1);

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            /* print instruction: [pos] instruction -> [register] */
            fprintf(stdout, " %3d: ", insn->pos);

            if (ir_has_value(insn) && insn->vreg != -1) {
                fprintf(stdout, "[R%d] ", insn->vreg);
            } else {
                fprintf(stdout, "[...] ");
            }

            klr_print_insn(insn, stdout);

            fprintf(stdout, "\n");
        }

        // print back-edge info for loop stretch
        KlrBasicBlock *succ;
        bb_succ_foreach(succ, bb) {
            if (succ == fn->ebb) continue; // skip end block
            if (succ->index <= bb->index) {
                fprintf(stdout, "      [BACK-EDGE] %s -> %s (Target POS: %d)\n", klr_block_name(bb),
                        klr_block_name(succ), succ->first_pos);
            }
        }
    }

    fprintf(stdout, "\n--- Max Register(local + call-args) ---\n");
    fprintf(stdout, "Max registers needed: %d + %d\n", fn->nlocals, fn->max_call_args);

    fprintf(stdout, "\n==========================================\n\n");
}

static void klr_assign_coord(KlrLSRAContext *ctx)
{
    KlrFunc *fn = ctx->func;

    int rpo_index = 0;
    int current_pos = 1; // Position 0 is reserved for parameters
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        // used for back-edge recognition
        bb->index = rpo_index++;

        // record the first pos of the block for live interval analysis
        bb->first_pos = current_pos;

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            // give lsra a monotonically increasing coordinate for register allocation
            insn->pos = current_pos++;
        }

        // record the last pos of the block for cross-block live range analysis
        bb->last_pos = current_pos;
    }

    ctx->last_pos = current_pos;
}

static void klr_mark_back_edges(KlrFunc *fn)
{
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrBasicBlock *succ;
        bb_succ_foreach(succ, bb) {
            // skip end block
            if (succ == fn->ebb) continue;
            if (succ->index <= bb->index) {
                bb->has_back_edge = 1;
                succ->is_loop_header = 1;
                log_info("Found Back-edge: %s (idx:%d) -> %s (idx:%d)", klr_block_name(bb),
                         bb->index, klr_block_name(succ), succ->index);
            }
        }
    }
}

/* a value whose register LSRA assigns: not a constant, not a fixed slot */
static int has_interval(KlrValue *val)
{
    if (klr_is_param(val)) return 1;
    return klr_is_insn(val) && !((KlrInsn *)val)->fixedslot;
}

/* the move `move %dst, %val` that is the only use of `val`, or NULL */
static KlrInsn *only_use_move(KlrValue *val)
{
    if (val->use_count != 1) return NULL;
    KlrUse *use;
    use_foreach(use, val) {
        KlrInsn *insn = use->insn;
        if (insn->code != OP_MOVE || insn_oper_value(insn, 1) != val) return NULL;
        return insn;
    }
    return NULL;
}

static int insn_reads(KlrInsn *insn, KlrValue *val)
{
    KlrValue *_val;
    insn_oper_value_foreach(_val, insn) {
        if (_val == val) return 1;
    }
    return 0;
}

/*
 * The interval of `val` if its register holds the same value over its whole life: an insn that
 * is not a local (not written by moves) and does not share the register of a local, or a
 * parameter that is never written (a self tail call writes the parameter slots).
 */
static KlrInterval *immutable_interval(KlrLSRAContext *ctx, KlrValue *val)
{
    if (klr_is_local(val)) return NULL;
    if (klr_is_param(val) && (val->def_count > 0 || ctx->func->has_tailcall)) return NULL;
    KlrInterval *intv = klr_value_interval(ctx, val);
    return (intv && !intv->alias) ? intv : NULL;
}

/*
 * Give the two sides of a move or a no-op cast the same register where it is safe, so cgen
 * emits nothing for it:
 *  A. `%x = local; move %x, %v`: %x is live from the move, where %v may end; it prefers the
 *     register of %v.
 *  B. `%c = cast %v` kept as a no-op cast: %c prefers the register of %v.
 *  D. ... and when %v is never written, %c shares its register even if %v lives on: the
 *     ranges of %c are added to %v.
 *  C. `%v = op ..; move %x, %v` with %x declared before: %v is written straight into the register
 *     of %x. The old value of %x is dead from %v on, as nothing but `local` lies in between and
 *     `op` does not read %x; the ranges of %v are added to %x.
 * A hint is only taken when the register is free, so A and B never change what is live.
 * The intervals are in the order params then insns by pos, so a value is seen before its uses.
 */
static void klr_coalesce(KlrLSRAContext *ctx)
{
    KlrInterval *intv;
    vector_foreach_ptr(intv, &ctx->intervals) {
        if (!klr_is_insn(intv->val)) continue;
        KlrInsn *insn = (KlrInsn *)intv->val;

        if (klr_is_local((KlrValue *)insn)) {
            KlrInsn *next = insn_next(insn, insn->bb);
            if (next && next->code == OP_MOVE && insn_oper_value(next, 0) == (KlrValue *)insn) {
                KlrValue *src = insn_oper_value(next, 1);
                if (has_interval(src)) intv->hint = src;
            }
            continue;
        }

        if (insn->code == OP_IR_CAST) {
            KlrValue *src = insn_oper_value(insn, 0);
            if (!has_interval(src)) continue;
            intv->hint = src;
            KlrInterval *sintv = immutable_interval(ctx, src);
            if (sintv) {
                /* D: both hold the same bits and neither is written again */
                intv->alias = src;
                klr_interval_union(sintv, intv);
            }
            continue;
        }

        if (klr_is_phi((KlrValue *)insn)) continue;

        KlrInsn *move = only_use_move((KlrValue *)insn);
        if (!move || move->bb != insn->bb) continue;
        KlrValue *dst = insn_oper_value(move, 0);
        if (!klr_is_local(dst) || ((KlrInsn *)dst)->fixedslot) continue;
        if (((KlrInsn *)dst)->pos >= insn->pos || insn_reads(insn, dst)) continue;

        KlrInsn *next = insn_next(insn, insn->bb);
        while (next && next != move && klr_is_local((KlrValue *)next)) {
            next = insn_next(next, insn->bb);
        }
        if (next != move) continue;

        KlrInterval *dintv = klr_value_interval(ctx, dst);
        if (!dintv) continue;

        log_info("coalesce %s into %s", klr_value_name((KlrValue *)insn), klr_value_name(dst));
        intv->alias = dst;
        klr_interval_union(dintv, intv);
    }
}

/* the interval that owns the register of `val` */
static KlrInterval *owner_interval(KlrLSRAContext *ctx, KlrValue *val)
{
    KlrInterval *intv = klr_value_interval(ctx, val);
    if (intv && intv->alias) intv = klr_value_interval(ctx, intv->alias);
    return intv;
}

static int interval_order(const void *a, const void *b)
{
    const KlrInterval *x = a, *y = b;
    if (x->start != y->start) return x->start < y->start ? -1 : 1;
    /* keep the build order (params, then insns by pos) for equal starts */
    int ix = x->val->tag, iy = y->val->tag;
    return ix < iy ? -1 : (ix > iy);
}

/* sort the intervals by start; the id -> interval map follows */
static void klr_sort_intervals(KlrLSRAContext *ctx)
{
    int n = vector_size(&ctx->intervals);
    if (n == 0) return;
    qsort(vector_get_ptr(&ctx->intervals, 0), n, sizeof(KlrInterval), interval_order);
    for (int i = 0; i < n; i++) {
        KlrInterval *intv = vector_get_ptr(&ctx->intervals, i);
        ctx->imap[intv->val->tag] = i;
    }
}

/* advance the cursor of `intv` to the first range not ended at `pos`; true if it covers `pos` */
static int interval_covers(KlrInterval *intv, int pos)
{
    while (intv->cursor < intv->nranges && intv->ranges[intv->cursor].end <= pos) intv->cursor++;
    return intv->cursor < intv->nranges && intv->ranges[intv->cursor].start <= pos;
}

static void __alloc_register(KlrLSRAContext *ctx, KlrInterval *intv)
{
    int reg = -1;
    KlrInterval *hint = intv->hint ? owner_interval(ctx, intv->hint) : NULL;
    if (hint && hint->val->vreg >= 0 && bitset_test(&ctx->bitset, hint->val->vreg)) {
        reg = hint->val->vreg;
        bitset_clear(&ctx->bitset, reg);
    } else {
        reg = bitset_ffs_and_clear(&ctx->bitset);
    }
    ASSERT(reg >= 0);
    intv->val->vreg = reg;
    int max = MAX(ctx->func->nlocals, reg + 1);
    ctx->func->nlocals = max;
}

/*
 * Linear scan over intervals with holes, without splitting. The intervals are taken by start;
 * at the start of each one:
 *  - active: the intervals covering the position, their registers are taken;
 *  - inactive: the intervals in a hole, their registers are free but must not be given to an
 *    interval that is live while they are.
 * An interval that ends at the start gives its register back, so a def can take the register
 * of an operand that dies there.
 */
static void klr_scan_and_alloc(KlrLSRAContext *ctx)
{
    int nintv = vector_size(&ctx->intervals);
    if (nintv == 0) return;

    KlrInterval **active = mm_alloc(sizeof(KlrInterval *) * nintv);
    KlrInterval **inactive = mm_alloc(sizeof(KlrInterval *) * nintv);
    int *blocked = mm_alloc(sizeof(int) * nintv);
    int nactive = 0, ninactive = 0;

    int start = -1;
    KlrInterval *cur;
    vector_foreach_ptr(cur, &ctx->intervals) {
        if (cur->alias) continue;

        int pos = cur->start;
        ASSERT(pos >= start);
        start = pos;

        for (int i = 0; i < nactive;) {
            KlrInterval *it = active[i];
            if (it->end <= pos || !interval_covers(it, pos)) {
                bitset_set(&ctx->bitset, it->val->vreg);
                active[i] = active[--nactive];
                if (it->end > pos) inactive[ninactive++] = it;
                else it->allocated = 0;
                continue;
            }
            i++;
        }

        for (int i = 0; i < ninactive;) {
            KlrInterval *it = inactive[i];
            if (it->end <= pos) {
                it->allocated = 0;
                inactive[i] = inactive[--ninactive];
                continue;
            }
            if (interval_covers(it, pos)) {
                ASSERT(bitset_test(&ctx->bitset, it->val->vreg));
                bitset_clear(&ctx->bitset, it->val->vreg);
                inactive[i] = inactive[--ninactive];
                active[nactive++] = it;
                continue;
            }
            i++;
        }

        /* the register of an inactive interval is not free if it is live again while cur is */
        int nblocked = 0;
        for (int i = 0; i < ninactive; i++) {
            KlrInterval *it = inactive[i];
            int reg = it->val->vreg;
            if (bitset_test(&ctx->bitset, reg) && klr_interval_intersects(it, cur)) {
                bitset_clear(&ctx->bitset, reg);
                blocked[nblocked++] = reg;
            }
        }

        __alloc_register(ctx, cur);
        cur->allocated = 1;
        active[nactive++] = cur;

        for (int i = 0; i < nblocked; i++) bitset_set(&ctx->bitset, blocked[i]);
    }

    /* an alias takes the register of its owner */
    vector_foreach_ptr(cur, &ctx->intervals) {
        if (!cur->alias) continue;
        KlrInterval *owner = klr_value_interval(ctx, cur->alias);
        ASSERT(owner && !owner->alias && owner->val->vreg >= 0);
        cur->val->vreg = owner->val->vreg;
    }

    mm_free(active);
    mm_free(inactive);
    mm_free(blocked);
}

static void klr_fixedslot_alloc(KlrLSRAContext *ctx)
{
    KlrInsn *insn;
    vector_foreach(insn, &ctx->fixed) {
        ASSERT(insn->fixedslot);
        if (insn->fixedslot == 1) {
            insn->vreg = ctx->func->nlocals + insn->slotindex;
            log_info("Assigning fixedslot register(next) %d to insn %s", insn->vreg,
                     klr_value_name((KlrValue *)insn));
        } else {
            ASSERT(insn->fixedslot == 2);
            insn->vreg = insn->slotindex;
            log_info("Assigning fixedslot register(pos) %d to insn %s", insn->vreg,
                     klr_value_name((KlrValue *)insn));
        }
    }
}


/*
 * Linear Scan Register Allocation:
 * a value is live over a list of ranges, see liveness.c.
 */
static void klr_lsra_run(KlrFunc *func)
{
    KlrLSRAContext ctx;

    vector_init(&ctx.intervals, sizeof(KlrInterval));
    vector_init_ptr(&ctx.fixed);
    ctx.func = func;

    // assign coordinate for each instruction in RPO order,
    // which serves as the time-axis for live interval analysis
    klr_assign_coord(&ctx);

    // mark back-edges, shown by the dump
    klr_mark_back_edges(func);

    /* the fixedslot insns get call slots, not registers */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn->fixedslot) vector_push_back(&ctx.fixed, &insn);
        }
    }

    /* live ranges of all insns and params in function */
    klr_build_live_ranges(&ctx);

    /* no more registers are live at once than there are intervals, so the scan never runs out;
     * the encoding limit is checked after the allocation. A whole row avoids padding bits. */
    int nbits = (vector_size(&ctx.intervals) + ROW_SIZE) / ROW_SIZE * ROW_SIZE;
    init_bitset(&ctx.bitset, nbits);
    // default all registers to free (1 means free, 0 means used)
    bitset_set_all(&ctx.bitset);

    /* share registers across moves and no-op casts */
    klr_coalesce(&ctx);

    klr_sort_intervals(&ctx);

    KlrInterval *intv;
    vector_foreach_ptr(intv, &ctx.intervals) {
        intv->val->vreg = -1;
    }

    /* perform linear scan register allocation */
    klr_scan_and_alloc(&ctx);

    /* perform fixedslot insn register allocation */
    klr_fixedslot_alloc(&ctx);

    /* a register operand is encoded in 8 bits */
    int nregs = func->nlocals + func->max_call_args;
    if (nregs > MAX_REGS) {
        KlrModule *m = func->module;
        KlrKlass *kls = func->klass;
        printf(ERROR_PREFIX "function '%s%s%s' needs %d registers (%d locals + %d call args), "
                            "more than %d\n",
               kls ? kls->name : "", kls ? "::" : "", func->name, nregs, func->nlocals,
               func->max_call_args, MAX_REGS);
        m->errors++;
    }

    if (dump_vreg_enabled()) {
        klr_lsra_dump(&ctx);
    }

    fini_bitset(&ctx.bitset);
    vector_foreach_ptr(intv, &ctx.intervals) {
        klr_interval_fini(intv);
    }
    vector_fini(&ctx.intervals);
    mm_free(ctx.vals);
    mm_free(ctx.imap);
    vector_fini(&ctx.fixed);
}

static void _do_lsra(KlrFunc *fn)
{
    klr_build_rpo(fn);

    // for simplicity, here add a return at the end of __init__ function
    if (str_equal(fn->name, "__init__")) {
        KlrBasicBlock *last = last_basic_block(fn);
        klr_add_last_return(last);
    }

    klr_lsra_run(fn);
}

void kl_do_lsra(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
        _do_lsra(fn);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            _do_lsra(fn);
        }
    }
}

#ifdef __cplusplus
}
#endif
