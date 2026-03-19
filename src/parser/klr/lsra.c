/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "lsra.h"
#include "bitset.h"
#include "cmd.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void klr_lsra_dump(KlrLSRAContext *ctx)
{
    KlrFunc *fn = ctx->func;

    fprintf(stdout, "\n=== LSRA DUMP: @%s ===\n", fn->name);

    fprintf(stdout, "--- Intervals ---");

    KlrInterval *intv;
    vector_foreach_ptr(intv, &ctx->intervals) {
        KlrValue *val = intv->val;

        fprintf(stdout, "\n%s: ", klr_value_name(val));
        fprintf(stdout, "range: [%d, %d) ", intv->start, intv->end);
        fprintf(stdout, "reg: %d", val->vreg);
    }

    fprintf(stdout, "\n--- Timeline & Registers ---\n");

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        fprintf(stdout, "bb: %-10s index: %d range: [%d, %d)\n", klr_block_name(bb),
                bb->index, bb->first_pos, bb->last_pos);

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            /* print instruction: [pos] instruction -> [register] */
            fprintf(stdout, "  %2d: ", insn->pos);

            if (ir_has_value(insn) && insn->vreg != -1) {
                fprintf(stdout, "[R%d] ", insn->vreg);
            } else {
                fprintf(stdout, "[----] ");
            }

            klr_print_insn(insn, stdout);

            fprintf(stdout, "\n");
        }

        fprintf(stdout, "%4d: AFTER-LAST\n", bb->last_pos);

        // print back-edge info for loop stretch
        KlrBasicBlock *succ;
        bb_succ_foreach(succ, bb) {
            if (succ == fn->ebb) continue; // skip end block
            if (succ->index <= bb->index) {
                fprintf(stdout, "      [BACK-EDGE] %s -> %s (Target POS: %d)\n",
                        klr_block_name(bb), klr_block_name(succ), succ->first_pos);
            }
        }
    }

    fprintf(stdout, "==========================================\n\n");
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
            if (succ->index <= bb->index) {
                bb->has_back_edge = 1;
                succ->is_loop_header = 1;
                log_debug("Found Back-edge: %s (idx:%d) -> %s (idx:%d)",
                          klr_block_name(bb), bb->index, klr_block_name(succ),
                          succ->index);
            }
        }
    }
}

static int val_last_use_pos(KlrValue *val)
{
    int max = 0;

    KlrUse *use;
    use_foreach(use, val) {
        /* use is not sorted by insn's pos, so we need to find the max value */
        KlrInsn *insn = use->insn;
        if (insn->pos > max) max = insn->pos;
    }

    return max;
}

static void klr_build_intervals(KlrLSRAContext *ctx)
{
    KlrFunc *fn = ctx->func;

    KlrParam *param;
    vector_foreach(param, &fn->params) {
        KlrInterval intv;
        intv.val = (KlrValue *)param;
        intv.allocated = 0;
        // parameters are defined at the beginning of the function
        intv.start = 0;
        intv.end = val_last_use_pos((KlrValue *)param);
        vector_push_back(&ctx->intervals, &intv);
    }

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (ir_has_value(insn)) {
                KlrInterval intv;
                intv.val = (KlrValue *)insn;
                intv.allocated = 0;
                intv.start = insn->pos;
                intv.end = val_last_use_pos((KlrValue *)insn);
                vector_push_back(&ctx->intervals, &intv);
            }
        }
    }
}

/**
 * Internal helper to stretch intervals of values live across a loop.
 * Returns true if any interval was extended (used for fixed-point iteration).
 */
static int klr_stretch_interval(KlrLSRAContext *ctx, int header_start, int loop_end)
{
    int changed = 0;
    KlrInterval *intv;

    vector_foreach(intv, &ctx->intervals) {
        /*
         * Logic: If a variable is live at the loop header,
         * stretch its lifetime to cover the entire loop body.
         */
        if (intv->start <= header_start && intv->end >= header_start) {
            if (intv->end < loop_end) {
                intv->end = loop_end;
                changed = 1;
            }
        }
    }
    return changed;
}
/**
 * Ensures intervals remain active across back-edges.
 * Uses reverse traversal to propagate liveness from inner to outer loops.
 */
void klr_fix_stretch_loop(KlrLSRAContext *ctx)
{
    KlrFunc *fn = ctx->func;
    int changed = 1;

    /* Fixed-point iteration ensures nested loops are fully stretched */
    while (changed) {
        changed = 0;
        KlrBasicBlock *bb;

        /* Process blocks in reverse order to speed up liveness propagation */
        bb_foreach_reverse(bb, fn) {
            /* Optimization: Only check blocks marked with a back-edge */
            if (!bb->has_back_edge) continue;

            KlrBasicBlock *succ;
            bb_succ_foreach(succ, bb) {
                /* Detect back-edge: target index is less than or equal to current */
                if (succ->index <= bb->index) {
                    if (klr_stretch_interval(ctx, succ->first_pos, bb->last_pos)) {
                        changed = 1;
                    }
                }
            }
        }
    }
}

static void __alloc_register(KlrLSRAContext *ctx, KlrValue *val)
{
    int reg = bitset_ffs_and_clear(&ctx->bitset);
    ASSERT(reg >= 0);
    val->vreg = reg;
}

static void __free_register(KlrLSRAContext *ctx, KlrValue *val)
{
    ASSERT(val->vreg >= 0);
    bitset_set(&ctx->bitset, val->vreg);
}

static void klr_scan_and_alloc(KlrLSRAContext *ctx)
{
#define in_range(i, v) (((i) >= (v)->start) && ((i) < (v)->end))

    int last_pos = ctx->last_pos;
    /* scan intervals(linear position: [0, last_pos) */
    for (int i = 0; i < last_pos; i++) {
        int start = -1;
        KlrInterval *intv;
        vector_foreach_ptr(intv, &ctx->intervals) {
            // TODO: sorted intervals:
            // here assert, when it ocurs, it means the intervals are not sorted by start
            // position, which may cause incorrect register allocation
            ASSERT(intv->start >= start);
            start = intv->start;

            /* Free registers for intervals that end at this position */
            if (!in_range(i, intv) && intv->allocated) {
                __free_register(ctx, intv->val);
                intv->allocated = 0;
            }

            /* Allocate registers for intervals that start at this position */
            if (in_range(i, intv) && !intv->allocated) {
                __alloc_register(ctx, intv->val);
                intv->allocated = 1;
            }
        }
    }
}

/*
 * Linear Scan Register Allocation:
 * variable interval = first definition point and last used point.
 */
void klr_lsra_run(KlrFunc *func)
{
    KlrLSRAContext ctx;

    vector_init(&ctx.intervals, sizeof(KlrInterval));
    init_bitset(&ctx.bitset, MAX_REGS);
    ctx.func = func;

    // default all registers to free (1 means free, 0 means used)
    bitset_set_all(&ctx.bitset);

    // assign coordinate for each instruction in RPO order,
    // which serves as the time-axis for live interval analysis
    klr_assign_coord(&ctx);

    // mark back-edges for loop stretch
    klr_mark_back_edges(func);

    /* build intervals for all insns and params in function */
    klr_build_intervals(&ctx);

    /* fix back-edge info for loop stretch */
    klr_fix_stretch_loop(&ctx);

    /* perform linear scan register allocation */
    klr_scan_and_alloc(&ctx);

    if (opt_dump_has(opt, DUMP_VREG)) {
        klr_lsra_dump(&ctx);
    }
}

#ifdef __cplusplus
}
#endif
