/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bitset.h"
#include "ir.h"
#include "mm.h"

/* codegen: register allocation */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlrInterval {
    /* the val is one of the three(param, var and inst) */
    KlrValue *val;
    /* the allocated flag used to control free register */
    int allocated;
    /* interval: [start, end), the interval of parameters are [0, end) */
    int start;
    int end;
} KlrInterval;

typedef struct _KlrLSRAContext {
    Vector intervals;
    BitSet bitset;
} KlrLSRAContext;

static void __alloc_register(KlrLSRAContext *ctx, KlrValue *val)
{
    int reg = bitset_ffs_and_clear(&ctx->bitset);
    ASSERT(reg >= 0);
    val->vreg = reg;
#ifndef NOLOG
    fprintf(stdout, "[alloc-register]: ");
    klr_print_name_or_tag(val, stdout);
    fprintf(stdout, ", reg: %d\n", reg);
#endif
}

static void __free_register(KlrLSRAContext *ctx, KlrValue *val)
{
    ASSERT(val->vreg >= 0);
    bitset_set(&ctx->bitset, val->vreg);
#ifndef NOLOG
    fprintf(stdout, "[free-register]: ");
    klr_print_name_or_tag(val, stdout);
    fprintf(stdout, ", reg: %d\n", val->vreg);
#endif
}

static int local_first_def_pos(KlrLocal *local)
{
    KlrUse *use = use_first(local);
    KlrInsn *insn = use->insn;
    ASSERT(insn->code == OP_IR_STORE);
    return insn->pos;
}

static int val_last_use_pos(KlrValue *val)
{
    int max = 0;
    KlrUse *use;
    use_foreach(use, val) {
        /* NOTE: use is not sorted by insn's pos */
        if (use->insn->pos > max) max = use->insn->pos;
    }
    return max;
}

/*
 * Linear Scan Register Allocation:
 * variable interval = first definition point and last used point.
 */
void klr_alloc_registers(KlrFunc *func)
{
    KlrLSRAContext ctx;
    vector_init(&ctx.intervals, sizeof(KlrInterval));
    init_bitset(&ctx.bitset, 256);
    bitset_set_all(&ctx.bitset);

    /* update instruction position */
    int pos = 0;
    KlrInsn *insn;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, func) {
        insn_foreach(insn, bb) {
            /* start with one, not zero, zero is for parameters */
            insn->pos = ++pos;
        }
    }

    /* parameters */
    KlrParam **param;
    vector_foreach(param, &func->params) {
        KlrInterval interval;
        interval.val = (KlrValue *)(*param);
        interval.allocated = 0;
        interval.start = 0;
        interval.end = val_last_use_pos((KlrValue *)(*param));
        vector_push_back(&ctx.intervals, &interval);
#ifndef NOLOG
        fprintf(stdout, "param: ");
        klr_print_name_or_tag((KlrValue *)(*param), stdout);
        fprintf(stdout, ", interval: [%d, %d)\n", interval.start, interval.end);
#endif
    }

    /* local variables */
    KlrLocal **local;
    vector_foreach(local, &func->locals) {
        KlrInterval interval;
        interval.val = (KlrValue *)(*local);
        interval.allocated = 0;
        interval.start = local_first_def_pos(*local);
        interval.end = val_last_use_pos((KlrValue *)(*local));
        vector_push_back(&ctx.intervals, &interval);
#ifndef NOLOG
        fprintf(stdout, "local: ");
        klr_print_name_or_tag((KlrValue *)(*local), stdout);
        fprintf(stdout, ", interval: [%d, %d)\n", interval.start, interval.end);
#endif
    }

    /* instructions */
    basic_block_foreach(bb, func) {
        insn_foreach(insn, bb) {
            if (insn_has_value(insn)) {
                KlrInterval interval;
                interval.val = (KlrValue *)insn;
                interval.allocated = 0;
                interval.start = insn->pos;
                interval.end = val_last_use_pos((KlrValue *)insn);
                vector_push_back(&ctx.intervals, &interval);
#ifndef NOLOG
                fprintf(stdout, "insn: ");
                klr_print_name_or_tag((KlrValue *)insn, stdout);
                fprintf(stdout, ", interval: [%d, %d)\n", interval.start, interval.end);
#endif
            }
        }
    }

#define in_range(i, v) (((i) >= (v)->start) && ((i) < (v)->end))

    /* scan intervals(linear position: [0, pos]) */
    int num_regs = vector_size(&ctx.intervals);
    ASSERT(num_regs < 256);
    for (int i = 0; i <= pos; i++) {
        KlrInterval *interval;
        for (int j = 0; j < num_regs; j++) {
            interval = vector_get(&ctx.intervals, j);
            KlrValue *val = interval->val;
            if (in_range(i, interval)) {
                if (!interval->allocated) {
                    __alloc_register(&ctx, val);
                    interval->allocated = 1;
                }
            } else {
                if (interval->allocated) {
                    __free_register(&ctx, val);
                    interval->allocated = 0;
                }
            }
        }
    }

#ifndef NOLOG
    KlrInterval *interval;
    for (int j = 0; j < num_regs; j++) {
        interval = vector_get(&ctx.intervals, j);
        fprintf(stdout, "value: ");
        klr_print_name_or_tag(interval->val, stdout);
        fprintf(stdout, "\n  interval: [%d, %d)", interval->start, interval->end);
        fprintf(stdout, "\n  reg: %d\n", interval->val->vreg);
    }
#endif
}

#ifdef __cplusplus
}
#endif
