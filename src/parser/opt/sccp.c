/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Sparse Conditional Constant Propagation (SCCP) for Koala SSA IR.
 *
 * Lattice (three-level, per SSA value):
 *   TOP    - undefined / not yet determined (initial state)
 *   CONST  - known constant value
 *   BOTTOM - overdefined / variable (non-constant)
 *
 * Meet rules (monotone, descending only):
 *   meet(TOP, x)    = x
 *   meet(BOTTOM, x) = BOTTOM
 *   meet(c, c)      = c          (same constant)
 *   meet(c1, c2)    = BOTTOM     (different constants)
 *
 * Two worklists drive the analysis:
 *   CFG worklist - edges (KlrEdge) to process for reachability
 *   SSA worklist - values (KlrValue) whose lattice descended
 *
 */

#include "atom.h"
#include "log.h"
#include "opt.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------ Lattice Value ------ */
typedef enum _LatticeKind {
    LATTICE_TOP,   // ⊤: no value known yet
    LATTICE_CONST, // constant value
    LATTICE_BOTTOM // ⊥: non-constant / overdefined
} LatticeKind;

typedef struct _LatticeValue {
    LatticeKind kind;
    /* valid only when kind == LAT_CONST */
    KlrValue *const_val;
} LatticeValue;

static inline LatticeValue lv_top(void) { return (LatticeValue){ LATTICE_TOP, NULL }; }
static inline LatticeValue lv_const(KlrValue *v) { return (LatticeValue){ LATTICE_CONST, v }; }
static inline LatticeValue lv_bottom(void) { return (LatticeValue){ LATTICE_BOTTOM, NULL }; }

/* Lattice meet operation.
 * Direction of information flow: TOP (Unknown) -> CONST -> BOTTOM (Conflict)
 */
static LatticeValue lv_meet(LatticeValue a, LatticeValue b)
{
    /* 1. TOP is the identity element: meet(TOP, x) = x */
    if (a.kind == LATTICE_TOP) return b;
    if (b.kind == LATTICE_TOP) return a;

    /* 2. BOTTOM is the annihilator: meet(BOTTOM, x) = BOTTOM */
    if (a.kind == LATTICE_BOTTOM || b.kind == LATTICE_BOTTOM) {
        return lv_bottom();
    }

    /* 3. Both are CONST: agree -> keep constant, disagree -> conflict (BOTTOM) */
    if (a.const_val == b.const_val) {
        return a;
    }

    return lv_bottom();
}

typedef struct _SCCPContext {
    KlrFunc *fn;
    /* KlrValue -> LatticeEntry */
    HashMap lattice;
    /* KlrEdge -> EdgeFlagEntry */
    HashMap exec_edges;
    /* edges to process */
    Queue cfg_wklist;
    /* values to re-evaluate */
    Queue ssa_wklist;
} SCCPContext;

/* ── Process one CFG worklist item ── */
static void process_cfg_edge(SCCPContext *ctx, KlrEdge *edge)
{
    KlrBasicBlock *dst_bb = edge->dst;

    /* Process all PHI nodes in destination block */
    KlrInsn *insn;
    insn_foreach(insn, dst_bb) {
        if (insn->code != OP_IR_PHI) break;
        LatticeValue new_lv = eval_insn(ctx, insn);
        update_value(ctx, (KlrValue *)insn, new_lv);
    }
}

static void sccp_analyze(SCCPContext *ctx)
{
    KlrFunc *fn = ctx->fn;

    /*
     * Seed the analysis: push the edge from the start-block sentinel
     * into the CFG worklist. This marks the real entry block reachable.
     */
    KlrEdge *entry_edge = edge_out_first(fn->sbb);
    if (entry_edge) {
        queue_push(&ctx->cfg_wklist, entry_edge);
    }

    /* Main analysis loop: alternate between CFG and SSA worklists */
    while (!queue_empty(&ctx->cfg_wklist) || !queue_empty(&ctx->ssa_wklist)) {
        while (!queue_empty(&ctx->cfg_wklist)) {
            KlrEdge *edge = queue_pop(&ctx->cfg_wklist);
            process_cfg_edge(ctx, edge);
        }

        while (!queue_empty(&ctx->ssa_wklist)) {
            KlrValue *val = queue_pop(&ctx->ssa_wklist);
            process_ssa_value(ctx, val);
        }
    }
}

int klr_sccp_pass(KlrFunc *fn)
{
    SCCPContext ctx;
    ctx.fn = fn;

    /* Clear visited flags from any previous pass */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    sccp_analyze(&ctx);

    return 0; // Placeholder return value for now
}

void kl_do_sccp(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
        klr_sccp_pass(fn);
        klr_dce_pass(fn, NULL);
        if (dump_ssa_enabled()) {
            fprintf(stdout, "--- IR Dump After sccp [@%s] ---\n", fn->name);
            klr_print_func(fn, stdout);
        }
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            klr_sccp_pass(fn);
            klr_dce_pass(fn, NULL);
            if (dump_ssa_enabled()) {
                fprintf(stdout, "--- IR Dump After sccp [@%s::%s] ---\n", kls->name, fn->name);
                klr_print_func(fn, stdout);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
