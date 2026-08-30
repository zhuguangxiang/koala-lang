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
#include "cmd.h"
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

static char *lv_str[] = {
    "TOP",
    "CONST",
    "BOTTOM",
};

typedef struct _LatticeValue {
    LatticeKind kind;
    /* valid only when kind == LAT_CONST */
    KlrValue *val;
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
    if (a.val == b.val) {
        return a;
    }

    return lv_bottom();
}

/* Check if two lattice values are equal. */
static bool lv_equal(LatticeValue a, LatticeValue b)
{
    if (a.kind != b.kind) return false;
    if (a.kind == LATTICE_CONST) return a.val == b.val;
    return true; /* TOP == TOP, BOTTOM == BOTTOM */
}

/* Lattice map entry: KlrValue -> LatticeValue */
typedef struct {
    HashMapEntry hnode;
    KlrValue *key;
    LatticeValue lv;
} LatticeEntry;

typedef struct _SCCPContext {
    KlrFunc *fn;
    /* KlrValue -> LatticeEntry */
    HashMap lattice;
    /* edges to process */
    Queue cfg_wklist;
    /* values to re-evaluate */
    Queue ssa_wklist;
} SCCPContext;

/** Get lattice value; returns TOP if no entry exists. */
LatticeValue get_lattice(SCCPContext *ctx, KlrValue *val)
{
    /* value is constant, no need for map lookup */
    if (klr_is_const(val)) return lv_const(val);

    /* value is parameter, no need for map lookup */
    if (klr_is_param(val)) return lv_bottom();

    LatticeEntry key = { .key = val };
    hashmap_entry_init(&key, mem_hash(&val, sizeof(val)));
    LatticeEntry *e = hashmap_get(&ctx->lattice, &key);
    return e ? e->lv : lv_top();
}

static bool set_lattice(SCCPContext *ctx, KlrValue *val, LatticeValue lv)
{
    if (lv.kind == LATTICE_TOP) {
        log_info("set_lattice: %s -> TOP, no need to set", klr_value_name(val));
        return false;
    }

    if (klr_is_const(val)) {
        log_info("set_lattice: %s is constant, no need to set", klr_value_name(val));
        return false;
    }

    if (klr_is_param(val)) {
        log_info("set_lattice: %s is parameter, no need to set", klr_value_name(val));
        return false;
    }

    LatticeEntry key = { .key = val };
    hashmap_entry_init(&key, mem_hash(&val, sizeof(val)));
    LatticeEntry *e = hashmap_get(&ctx->lattice, &key);
    if (e) {
        log_info("set_lattice: %s current lattice %s, new lattice %s", klr_value_name(val),
                 lv_str[e->lv.kind], lv_str[lv.kind]);
        LatticeValue merged = lv_meet(e->lv, lv);
        if (lv_equal(e->lv, merged)) {
            log_info("set_lattice: %s lattice %s is not changed", klr_value_name(val),
                     lv_str[lv.kind]);
            return false;
        }
        log_info("set_lattice: %s lattice %s -> %s", klr_value_name(val), lv_str[e->lv.kind],
                 lv_str[merged.kind]);
        e->lv = merged;
        return true;
    }

    e = mm_alloc_obj(e);
    hashmap_entry_init(e, mem_hash(&val, sizeof(val)));
    e->key = val;
    e->lv = lv;
    hashmap_put_only(&ctx->lattice, e);
    log_info("set_lattice: %s lattice %s -> %s", klr_value_name(val), lv_str[LATTICE_TOP],
             lv_str[lv.kind]);
    return true;
}

static LatticeValue fold_binary(SCCPContext *ctx, KlrInsn *insn, LatticeValue lv, LatticeValue rv)
{
    /* Only fold when both operands are known constants */
    if (lv.kind != LATTICE_CONST || rv.kind != LATTICE_CONST) return lv_bottom();

    KlrConst *lc = (KlrConst *)lv.val;
    KlrConst *rc = (KlrConst *)rv.val;
    KlrModule *m = ctx->fn->module;
    TypeSpec *ts = insn->ts;

    switch (insn->code) {
        case OP_BINARY_ADD: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t result = (int64_t)lc->ival + (int64_t)rc->ival;
                return lv_const(klr_const_int((uint64_t)result, ts, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t result = lc->ival + rc->ival;
                return lv_const(klr_const_uint(result, ts, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT)
                return lv_const(klr_const_float(lc->fval + rc->fval, ts, m));
            return lv_bottom();
        }

        case OP_BINARY_SUB: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t result = (int64_t)lc->ival - (int64_t)rc->ival;
                return lv_const(klr_const_int((uint64_t)result, ts, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t result = lc->ival - rc->ival;
                return lv_const(klr_const_uint(result, ts, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT)
                return lv_const(klr_const_float(lc->fval - rc->fval, ts, m));
            return lv_bottom();
        }

        case OP_BINARY_MUL: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t result = (int64_t)lc->ival * (int64_t)rc->ival;
                return lv_const(klr_const_int((uint64_t)result, ts, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t result = lc->ival * rc->ival;
                return lv_const(klr_const_uint(result, ts, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT)
                return lv_const(klr_const_float(lc->fval * rc->fval, ts, m));
            return lv_bottom();
        }

        case OP_BINARY_DIV: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                if (b == 0) return lv_bottom();
                if (a == INT64_MIN && b == -1) return lv_bottom();
                return lv_const(klr_const_int((uint64_t)(a / b), ts, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                if (b == 0) return lv_bottom();
                return lv_const(klr_const_uint(a / b, ts, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                if (rc->fval == 0.0) return lv_bottom();
                return lv_const(klr_const_float(lc->fval / rc->fval, ts, m));
            }
            return lv_bottom();
        }

        case OP_BINARY_GT: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                return lv_const(klr_const_bool(a > b, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                return lv_const(klr_const_bool(a > b, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                return lv_const(klr_const_bool(lc->fval > rc->fval, m));
            }
            return lv_bottom();
        }

        case OP_BINARY_LT: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                return lv_const(klr_const_bool(a < b, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                return lv_const(klr_const_bool(a < b, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                return lv_const(klr_const_bool(lc->fval < rc->fval, m));
            }
            return lv_bottom();
        }

        case OP_BINARY_EQ: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                return lv_const(klr_const_bool(a == b, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                return lv_const(klr_const_bool(a == b, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                return lv_const(klr_const_bool(lc->fval == rc->fval, m));
            }
            return lv_bottom();
        }

        case OP_BINARY_GE: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                return lv_const(klr_const_bool(a >= b, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                return lv_const(klr_const_bool(a >= b, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                return lv_const(klr_const_bool(lc->fval >= rc->fval, m));
            }
            return lv_bottom();
        }

        case OP_BINARY_LE: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                return lv_const(klr_const_bool(a <= b, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                return lv_const(klr_const_bool(a <= b, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                return lv_const(klr_const_bool(lc->fval <= rc->fval, m));
            }
            return lv_bottom();
        }

        case OP_BINARY_NE: {
            if (lc->which == CONST_INT && rc->which == CONST_INT) {
                int64_t a = (int64_t)lc->ival;
                int64_t b = (int64_t)rc->ival;
                return lv_const(klr_const_bool(a != b, m));
            }
            if (lc->which == CONST_UINT && rc->which == CONST_UINT) {
                uint64_t a = lc->ival;
                uint64_t b = rc->ival;
                return lv_const(klr_const_bool(a != b, m));
            }
            if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
                return lv_const(klr_const_bool(lc->fval != rc->fval, m));
            }
            return lv_bottom();
        }

        default:
            return lv_bottom();
    }
}

static LatticeValue eval_insn(SCCPContext *ctx, KlrInsn *insn)
{
    /* ---- Phi: meet of operands from executable predecessors ---- */
    if (insn->code == OP_IR_PHI) {
        LatticeValue result = lv_top();
        int32_t has_exec_pred = 0;

        for (int32_t i = 0; i < (int32_t)insn->filled; i++) {
            KlrBasicBlock *pred = insn->phi_preds[i];
            KlrEdge *e = klr_find_edge(pred, insn->bb);
            if (e && e->visited) {
                has_exec_pred = 1;
                LatticeValue oper_lv = get_lattice(ctx, insn_oper_value(insn, i));
                result = lv_meet(result, oper_lv);
                if (result.kind == LATTICE_BOTTOM) break;
            }
        }

        if (!has_exec_pred) return lv_top();
        return result;
    }

    /* ---- Binary arithmetic / comparison ---- */
    if (insn->code >= OP_BINARY_ADD && insn->code <= OP_BINARY_GE) {
        KlrValue *lhs_val = insn_oper_value(insn, 0);
        KlrValue *rhs_val = insn_oper_value(insn, 1);
        LatticeValue lhs = get_lattice(ctx, lhs_val);
        LatticeValue rhs = get_lattice(ctx, rhs_val);
        return fold_binary(ctx, insn, lhs, rhs);
    }

    /* ---- Local declaration: does not produce a value ---- */
    if (insn->code == OP_IR_LOCAL) {
        return lv_top();
    }

    /* ---- Unary operations ---- */
    // if (insn->code >= OP_UNARY_PLUS && insn->code <= OP_UNARY_NOT) {
    //     LatticeValue operand = get_lattice(ctx, insn_oper_value(insn, 0));
    //     if (operand.tag == LATTICE_BOTTOM) return lv_bottom();
    //     if (operand.tag == LATTICE_TOP) return lv_top();

    //     KlrConst *c = (KlrConst *)operand.val;
    //     KlrModule *m = ctx->func->module;

    //     switch (insn->code) {
    //         case OP_UNARY_PLUS:
    //             return operand;
    //         case OP_UNARY_NEG:
    //             if (c->which == CONST_INT) {
    //                 int64_t result = -(int64_t)c->ival;
    //                 return lv_const(klr_const_int((uint64_t)result, insn->ts, m));
    //             }
    //             if (c->which == CONST_FLT) return lv_const(klr_const_float(-c->fval, insn->ts,
    //             m)); return lv_bottom();
    //         case OP_UNARY_NOT:
    //             if (c->which == CONST_BOOL) return lv_const(klr_const_bool(!c->bval, m));
    //             return lv_bottom();
    //         default:
    //             return lv_bottom();
    //     }
    // }

    /* ---- Select: cond ? true_val : false_val ---- */
    if (insn->code == OP_IR_SELECT) {
        KlrValue *cond_val = insn_oper_value(insn, 0);
        LatticeValue cond = get_lattice(ctx, cond_val);
        if (cond.kind == LATTICE_BOTTOM) return lv_bottom();
        if (cond.kind == LATTICE_CONST) {
            KlrConst *cc = (KlrConst *)cond.val;
            ASSERT(cc->which == CONST_BOOL);
            KlrValue *selected_val =
                cc->bval ? insn_oper_value(insn, 1) : insn_oper_value(insn, 2);
            return get_lattice(ctx, selected_val);
        }
        /* cond is TOP: return TOP, do not speculatively descend */
        return lv_top();
    }

    /* ---- Cast: try constant folding ---- */
    // if (insn->code == OP_IR_CAST) {
    //     LatticeValue operand = get_lattice(ctx, insn_oper_value(insn, 0));
    //     if (operand.tag == LATTICE_BOTTOM) return lv_bottom();
    //     if (operand.tag == LATTICE_TOP) return lv_top();
    //     /* Constant folding for casts is complex (width changes, overflow
    //      * checks); conservatively return BOTTOM for analysis. */
    //     return lv_bottom();
    // }

    /* ---- Calls, loads, and everything else: conservative BOTTOM ---- */
    return lv_bottom();
}

static void push_use_to_eval_wklist(SCCPContext *ctx, KlrValue *val)
{
    log_info("push_use_to_eval_wklist: value %s lattice %s", klr_value_name(val),
             lv_str[get_lattice(ctx, val).kind]);

    KlrUse *use;
    use_foreach(use, val) {
        if (use->is_def) continue;
        KlrInsn *_insn = use->insn;
        if (!_insn) continue;

        if (klr_is_phi((KlrValue *)_insn)) {
            /*
             * PHI nodes: Only schedule if the incoming edge carrying this
             * value is reachable. We break after the first reachable match
             * since pushing the same PHI multiple times is redundant.
             */
            for (int i = 0; i < _insn->num_opers; i++) {
                if (insn_oper_value(_insn, i) == val) {
                    KlrBasicBlock *pred = _insn->phi_preds[i];
                    KlrEdge *e = klr_find_edge(pred, _insn->bb);
                    if (e && e->visited) {
                        log_info("value %s is pushed to re-eval wklist (PHI)",
                                 klr_value_name((KlrValue *)_insn));
                        queue_push(&ctx->ssa_wklist, _insn);
                        break;
                    }
                }
            }
        } else {
            /* Non-PHI instructions: unconditionally schedule for re-evaluation. */
            log_info("value %s is pushed to re-eval wklist", klr_value_name((KlrValue *)_insn));
            queue_push(&ctx->ssa_wklist, _insn);
        }
    }
}

/*
 * Evaluate a terminator instruction and push executable successor edges into the CFG worklist.
 */
static void evaluate_terminator(SCCPContext *ctx, KlrInsn *insn)
{
    switch (insn->code) {
        case OP_JMP: {
            /* Unconditional: all outgoing edges are executable */
            KlrEdge *e;
            edge_out_foreach(e, insn->bb) {
                if (e && !e->visited) {
                    e->visited = true;
                    log_info("edge %s -> %s is pushed to CFG wklist (unconditional jump)",
                             klr_block_name(e->src), klr_block_name(e->dst));
                    queue_push(&ctx->cfg_wklist, e);
                }
            }
            break;
        }

        case OP_IR_JMP_COND: {
            /* Conditional jump: reachability depends on the condition's lattice state */
            KlrValue *cond_val = insn_oper_value(insn, 0);
            KlrBasicBlock *then_bb = insn_oper_value_as_bb(insn, 2);
            KlrBasicBlock *else_bb = insn_oper_value_as_bb(insn, 3);

            LatticeValue cond_lv = get_lattice(ctx, cond_val);

            if (cond_lv.kind == LATTICE_CONST) {
                /* Condition is a known constant: only the taken branch is reachable */
                KlrConst *cc = (KlrConst *)cond_lv.val;
                bool is_true = (cc->which == CONST_BOOL) && cc->bval;
                KlrBasicBlock *taken_bb = is_true ? then_bb : else_bb;

                KlrEdge *e = klr_find_edge(insn->bb, taken_bb);
                if (e && !e->visited) {
                    e->visited = true;
                    log_info("bb %s is pushed to CFG wklist(cond is const %s)",
                             klr_block_name(taken_bb), is_true ? "true" : "false");
                    queue_push(&ctx->cfg_wklist, e);
                }

            } else if (cond_lv.kind == LATTICE_BOTTOM) {
                /* Condition is varying/conflict: both branches are potentially reachable */
                KlrEdge *e;
                edge_out_foreach(e, insn->bb) {
                    if (e && !e->visited) {
                        e->visited = true;
                        log_info(
                            "bb %s is pushed to CFG wklist(cond is BOTTOM, both branches "
                            "reachable)",
                            klr_block_name(e->src));
                        queue_push(&ctx->cfg_wklist, e);
                    }
                }
            }
            /* If LATTICE_TOP: condition is unknown, no edges are reachable yet. Do nothing. */
            break;
        }

        default: {
            // OP_RET & OP_RET_VOID: do not have outgoing CFG edges
            ASSERT(insn->code == OP_RET || insn->code == OP_RET_VOID);
            log_info("terminator %s has no outgoing edges, nothing to push to CFG wklist",
                     klr_value_name((KlrValue *)insn));
            break;
        }
    }
}

/* first reach: evaluate everything in the block */
static void process_all_insns(SCCPContext *ctx, KlrBasicBlock *bb)
{
    bb->visited = 1;
    log_info("[SCCP] block %s becomes reachable", klr_block_name(bb));

    /* Evaluate all PHIs (full evaluation) */
    KlrInsn *insn;
    insn_foreach(insn, bb) {
        if (!klr_is_phi((KlrValue *)insn)) break;
        LatticeValue lv = eval_insn(ctx, insn);
        if (set_lattice(ctx, (KlrValue *)insn, lv)) {
            push_use_to_eval_wklist(ctx, (KlrValue *)insn);
        }
    }

    /* Evaluate non-PHI data instructions */
    insn_foreach(insn, bb) {
        if (klr_is_phi((KlrValue *)insn)) continue;

        if (insn_is_terminator(insn)) {
            ASSERT(insn_last(bb) == insn);
            evaluate_terminator(ctx, insn);
            break; /* terminator is the last instruction */
        }

        /* Special case: move propagates source lattice to target local */
        if (insn->code == OP_MOVE) {
            KlrValue *target = insn_oper_value(insn, 0); /* local */
            KlrValue *source = insn_oper_value(insn, 1); /* value */
            LatticeValue src_lv = get_lattice(ctx, source);
            if (set_lattice(ctx, (KlrValue *)target, src_lv)) {
                push_use_to_eval_wklist(ctx, (KlrValue *)target);
            }
            continue;
        }

        LatticeValue lv = eval_insn(ctx, insn);
        if (set_lattice(ctx, (KlrValue *)insn, lv)) {
            push_use_to_eval_wklist(ctx, (KlrValue *)insn);
        }
    }
}

/* Incrementally update PHI nodes when a new incoming edge becomes reachable */
static void process_phi_incr(SCCPContext *ctx, KlrBasicBlock *pred, KlrBasicBlock *bb)
{
    log_info(
        "[SCCP] block %s is already reachable, incrementally update PHIs with new edge from %s",
        klr_block_name(bb), klr_block_name(pred));

    KlrInsn *insn;

    /* PHIs are guaranteed to be at the very beginning of the basic block */
    insn_foreach(insn, bb) {
        if (!klr_is_phi((KlrValue *)insn)) break;

        /* Short-circuit optimization: if PHI is already BOTTOM, it cannot descend further */
        LatticeValue old_lv = get_lattice(ctx, (KlrValue *)insn);
        if (old_lv.kind == LATTICE_BOTTOM) continue;

        /*
         * Calculate the lattice contribution exclusively from this newly reachable edge.
         * (Handles the rare case where a single edge carries multiple operands for the same PHI).
         */
        LatticeValue edge_contrib = lv_top();
        for (int i = 0; i < insn->num_opers; i++) {
            if (insn->phi_preds[i] == pred) {
                KlrValue *_v = insn_oper_value(insn, i);
                LatticeValue op_lv = get_lattice(ctx, _v);
                edge_contrib = lv_meet(edge_contrib, op_lv);
            }
        }

        /*
         * Pass the edge's contribution to set_lattice.
         * set_lattice will internally compute: meet(old_lv, edge_contrib).
         * This avoids redundant meet operations and keeps the logic extremely clean.
         */
        if (set_lattice(ctx, (KlrValue *)insn, edge_contrib)) {
            push_use_to_eval_wklist(ctx, (KlrValue *)insn);
        }
    }

    log_info("[SCCP] finished incrementally updating PHIs for block %s", klr_block_name(bb));
}

/* Process one CFG worklist item */
static void process_cfg_edge(SCCPContext *ctx, KlrEdge *edge)
{
    KlrBasicBlock *bb = edge->dst;

    if (bb->visited) {
        /* Block already visited: incrementally update PHIs with new edge's operands */
        KlrBasicBlock *pred = edge->src;
        process_phi_incr(ctx, pred, bb);
    } else {
        /* First time visiting this block: full evaluation of all instructions */
        process_all_insns(ctx, bb);
    }
}

static void process_ssa_value(SCCPContext *ctx, KlrValue *val)
{
    log_info("re-eval value %s", klr_value_name(val));

    if (!klr_is_insn(val)) return;

    KlrInsn *insn = (KlrInsn *)val;

    if (insn_is_terminator(insn)) {
        /* Re-evaluate terminator: may discover new executable edges
         * when the branch condition descends from TOP to CONST/BOTTOM. */
        if (!insn->bb->visited) return;
        evaluate_terminator(ctx, insn);
        return;
    }

    /* Special case: move does not produce an SSA value.
     * It writes source lattice to the target local. */
    if (insn->code == OP_MOVE) {
        KlrValue *target = insn_oper_value(insn, 0); /* local */
        KlrValue *source = insn_oper_value(insn, 1); /* value */
        LatticeValue src_lv = get_lattice(ctx, source);
        if (set_lattice(ctx, target, src_lv)) {
            log_info("re-eval move: target %s lattice updated to %s", klr_value_name(target),
                     lv_str[src_lv.kind]);
            push_use_to_eval_wklist(ctx, target);
        }
        return;
    }

    /* Re-evaluate instruction */
    LatticeValue new_lv = eval_insn(ctx, insn);
    if (set_lattice(ctx, val, new_lv)) {
        log_info("re-eval insn: %s lattice updated to %s", klr_value_name(val),
                 lv_str[new_lv.kind]);
        push_use_to_eval_wklist(ctx, val);
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
        ASSERT(!entry_edge->visited);
        entry_edge->visited = 1;
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

/*
 * Replace constants via RAUW
 *
 * For each SSA value whose lattice is CONST, replace all uses
 * with the known constant. Skip phi nodes (handled later by DCE
 * after branch folding) and values that are already constants.
 */
static void replace_constants(SCCPContext *ctx, KlrFunc *fn)
{
    KlrBasicBlock *bb;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn, *nxt_insn;
        insn_foreach_safe(insn, nxt_insn, bb) {
            /* Skip phi nodes — they'll be cleaned up by DCE */
            if (klr_is_phi((KlrValue *)insn)) continue;

            /* Skip terminators — branch folding handles them */
            if (insn_is_terminator(insn)) continue;

            /* Check if this instruction has a known constant lattice value */
            LatticeValue lv = get_lattice(ctx, (KlrValue *)insn);
            if (lv.kind != LATTICE_CONST) continue;

            log_info("[sccp-rewrite] replace const:");
            log_insn(insn);

            /* RAUW: replace all uses of this insn with the constant */
            replace_all_uses_with(lv.val, (KlrValue *)insn);
        }
    }
}

/**
 * Remove all phi operands in @succ that reference @pred as predecessor.
 * Used when an edge pred→succ is being removed (branch folding,
 * unreachable block removal, etc.) to maintain SSA invariant.
 */
static void remove_phi_operands_for_pred(KlrBasicBlock *succ, KlrBasicBlock *pred)
{
    KlrInsn *insn;
    insn_foreach(insn, succ) {
        if (!klr_is_phi((KlrValue *)insn)) break;

        for (int i = 0; i < insn->filled; i++) {
            if (insn->phi_preds[i] != pred) continue;

            /* Clear operand (decrements use_count) */
            clear_operand_at(insn, i);

            /* Shift last entry into slot i */
            int last = insn->filled - 1;
            if (i != last) {
                insn->phi_preds[i] = insn->phi_preds[last];
                set_operand_at(insn, i, insn_oper_value(insn, last));
            }
            clear_operand_at(insn, last);
            insn->filled--;
            i--; /* re-check slot i after swap */
        }
    }
}

/*
 * Fold conditional branches
 *
 * If a conditional branch has a known-constant condition, fold it
 * to an unconditional jump to the taken branch.
 */
static int fold_branches(SCCPContext *ctx, KlrFunc *fn)
{
    KlrBasicBlock *bb;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn = insn_last(bb);
        if (!insn || insn->code != OP_IR_JMP_COND) continue;

        /* Read lattice of the condition operand */
        LatticeValue lv = get_lattice(ctx, insn_oper_value(insn, 0));
        if (lv.kind != LATTICE_CONST) continue;

        KlrConst *cc = (KlrConst *)lv.val;
        if (cc->which != CONST_BOOL) continue;

        int taken = cc->bval;
        /* Operand 2 = then block, operand 3 = else block */
        int index = taken ? 2 : 3;
        KlrBasicBlock *dst = insn_oper_value_as_bb(insn, index);
        ASSERT(dst->kind == KLR_VALUE_BLOCK);

        log_info("[sccp-rewrite] fold branch in '%%%s' -> '%%%s' (cond=%d)", klr_block_name(bb),
                 klr_block_name(dst), taken);

        index = taken ? 3 : 2;
        KlrBasicBlock *dropped_bb = insn_oper_value_as_bb(insn, index);
        if (dst != dropped_bb) {
            remove_phi_operands_for_pred(dropped_bb, bb);
        }

        /* Remove all outgoing edges and the conditional jump */
        klr_remove_all_out_edges(bb);
        klr_erase_insn(insn);

        /* Add unconditional jump to the target */
        KlrBuilder bldr;
        klr_builder_end(&bldr, bb);
        klr_build_jmp(&bldr, dst);
    }
}

/*
 * Fold select instructions whose condition is a known constant.
 * Replaces the select with its chosen operand and removes the instruction.
 */
static int fold_select(KlrFunc *fn)
{
    int changed = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn, *nxt;
        insn_foreach_safe(insn, nxt, bb) {
            if (insn->code != OP_IR_SELECT) continue;

            KlrValue *cond = insn_oper_value(insn, 0);
            if (!klr_is_const(cond)) continue;
            KlrConst *c = (KlrConst *)cond;
            if (c->which != CONST_BOOL) continue;

            int idx = c->bval ? 1 : 2;
            KlrValue *selected = insn_oper_value(insn, idx);

            log_info("[fold-select] replacing select with %s", klr_value_name(selected));
            replace_all_uses_with(selected, (KlrValue *)insn);
            // klr_erase_insn(insn);
            changed = 1;
        }
    }
    return changed;
}

/*
 * Remove unreachable blocks
 *
 * BFS from the entry block (via sbb) to mark reachable blocks,
 * then erase any unvisited blocks.
 */
static int remove_unreachable_blocks(KlrFunc *fn)
{
    int changed = 0;

    /* Clear visited flags */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    /* BFS from sbb to mark reachable blocks */
    QUEUE(wklist);
    fn->sbb->visited = 1;
    queue_push(&wklist, fn->sbb);

    while (!queue_empty(&wklist)) {
        KlrBasicBlock *cur = queue_pop(&wklist);
        KlrBasicBlock *succ;
        bb_succ_foreach(succ, cur) {
            if (succ == fn->ebb) continue;
            if (!succ->visited) {
                succ->visited = 1;
                queue_push(&wklist, succ);
            }
        }
    }

    /* Phase 1: Clean up phi nodes in reachable successors of unreachable blocks */
    basic_block_foreach(bb, fn) {
        if (!bb->visited) { /* bb is unreachable */
            KlrEdge *edge, *nxt_edge;
            edge_out_foreach_safe(edge, nxt_edge, bb) {
                KlrBasicBlock *succ = edge->dst;
                if (succ->visited) { /* succ is reachable - its phis need cleanup */
                    KlrInsn *insn;
                    insn_foreach(insn, succ) {
                        if (!klr_is_phi((KlrValue *)insn)) break;
                        /* Find and remove the phi operand from predecessor bb */
                        for (int i = 0; i < insn->filled; i++) {
                            if (insn->phi_preds[i] == bb) {
                                /* Clear the operand first (decrements use_count) */
                                clear_operand_at(insn, i);
                                /* Shift remaining entries down */
                                int last = insn->filled - 1;
                                if (i != last) {
                                    insn->phi_preds[i] = insn->phi_preds[last];
                                    /* Move the last operand to position i */
                                    set_operand_at(insn, i, insn_oper_value(insn, last));
                                }
                                clear_operand_at(insn, last);
                                insn->filled--;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Phase 2: Now safe to erase unreachable blocks */
    KlrBasicBlock *nxt;
    basic_block_foreach_safe(bb, nxt, fn) {
        if (!bb->visited) {
            log_info("[sccp-rewrite] erase unreachable block '%%%s'", klr_block_name(bb));
            klr_erase_block(bb);
            changed = 1;
        }
        bb->visited = 0;
    }

    fn->sbb->visited = 0;
    return changed;
}

/*
 * Simplify conditional branches where both targets are the same block.
 * This can happen after other CFG optimizations (e.g., remove_only_jump_block).
 * Returns 1 if any branch was folded, 0 otherwise.
 */
static int fold_identical_branches(KlrFunc *fn)
{
    int changed = 0;
    KlrBasicBlock *bb;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn = insn_last(bb);
        if (!insn || insn->code != OP_IR_JMP_COND) continue;

        KlrBasicBlock *then_bb = (KlrBasicBlock *)insn_oper_value(insn, 2);
        KlrBasicBlock *else_bb = (KlrBasicBlock *)insn_oper_value(insn, 3);

        if (then_bb != else_bb) continue;

        /* Safety: if the target block has any PHI, we cannot fold blindly.
         * (The original klr_remove_only_jump_block will preserve such blocks,
         *  but we check here anyway.)
         */
        // KlrInsn *first = insn_first(then_bb);
        // if (first && first->code == OP_IR_PHI) {
        //     log_info("[fold-identical] skip folding, target block has PHI");
        //     continue;
        // }

        log_info("[simplify] fold identical branch in '%%%s' -> '%%%s'", klr_block_name(bb),
                 klr_block_name(then_bb));

        klr_remove_all_out_edges(bb);
        klr_erase_insn(insn);
        KlrBuilder bldr;
        klr_builder_end(&bldr, bb);
        klr_build_jmp(&bldr, then_bb);
        changed = 1;
    }

    return changed;
}

/*
 * For each basic block that has exactly one predecessor, replace all its PHI nodes
 * with their sole operand and delete the PHI nodes. This simplifies the IR and
 * avoids issues when merging blocks with a PHI that references the merging destination.
 */
static int simplify_single_pred_phi(KlrFunc *fn)
{
    int changed = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        if (bb->num_inedges != 1) continue;
        KlrEdge *in = edge_in_first(bb);
        KlrBasicBlock *pred = in->src;

        KlrInsn *insn, *nxt;
        insn_foreach_safe(insn, nxt, bb) {
            if (insn->code != OP_IR_PHI) break;
            // This PHI must have exactly one operand (since only one predecessor)
            ASSERT(insn->filled == 1 && insn->phi_preds[0] == pred);
            KlrValue *val = insn_oper_value(insn, 0);
            // Replace all uses of this PHI with its operand
            replace_all_uses_with(val, (KlrValue *)insn);
            // Erase the PHI instruction
            klr_erase_insn(insn);
            changed = 1;
        }
    }
    return changed;
}

static void sccp_rewrite(SCCPContext *ctx)
{
    replace_constants(ctx, ctx->fn);
    fold_branches(ctx, ctx->fn);
    fold_select(ctx->fn);

    int changed = 1;
    while (changed) {
        changed = 0;
        changed |= fold_identical_branches(ctx->fn);
        changed |= remove_unreachable_blocks(ctx->fn);
        changed |= klr_remove_only_jump_block(ctx->fn, NULL);
        changed |= simplify_single_pred_phi(ctx->fn);
        changed |= klr_merge_block(ctx->fn, NULL);
        changed |= klr_dce_pass(ctx->fn, NULL);
    }
}

static int __lattice_equal__(void *a, void *b)
{
    return ((LatticeEntry *)a)->key == ((LatticeEntry *)b)->key;
}

int klr_sccp_pass(KlrFunc *fn)
{
    SCCPContext ctx;
    ctx.fn = fn;
    init_queue(&ctx.cfg_wklist);
    init_queue(&ctx.ssa_wklist);
    hashmap_init(&ctx.lattice, __lattice_equal__);

    /* Clear visited flags from any previous pass */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    sccp_analyze(&ctx);
    sccp_rewrite(&ctx);

    return 0;
}

void kl_do_sccp(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
        klr_sccp_pass(fn);
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
