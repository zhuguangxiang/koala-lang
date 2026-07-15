/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/**
 * SCCP Rewrite Pass - Transform IR using SCCP analysis results.
 *
 * This pass queries the lattice values computed by sccp.c and performs:
 *   1. Constant replacement (RAUW for values with known constants)
 *   2. Conditional branch folding (when condition is known constant)
 *   3. Unreachable block removal (BFS from entry)
 *   4. Dead instruction elimination (cascading DCE)
 *   5. RPO rebuild
 */

#include "log.h"
#include "opt.h"
#include "queue.h"
#include "sccp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * Side-effect check for DCE
 * ================================================================ */

static int has_side_effect(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_GLOBAL_SET:
        case OP_RET:
        case OP_RET_VOID:
        case OP_IR_JMP_COND:
        case OP_JMP:
        case OP_IR_LOCAL:
        case OP_NEW:
        case OP_SET_FIELD:
        case OP_SET_FIELD_EXT:
        case OP_SEQ_SET:
        case OP_MAP_SET:
        case OP_SEQ_SET_IMM:
            return 1;

        case OP_IR_CALL: {
            if (insn->flags & KLR_INSN_FLAGS_CONST) {
                KlrValue *callee = insn_oper_value(insn, 0);
                if (callee->kind == KLR_VALUE_KLASS) {
                    return 0;
                }
            }
            return 1;
        }

        case OP_MOVE: {
            KlrValue *_dst = insn_oper_value(insn, 0);
            KlrValue *src = insn_oper_value(insn, 1);
            ASSERT(klr_is_local(_dst));
            KlrInsn *dst = (KlrInsn *)_dst;
            if (_dst == src) return 0;
            if (dst->use_count == 0) return 0;
            if (dst->flags & KLR_INSN_FLAGS_CONST) {
                if (klr_is_const(src)) return 0;
                if (klr_is_local(src) && (((KlrInsn *)src)->flags & KLR_INSN_FLAGS_CONST))
                    return 0;
                if (src->kind == KLR_VALUE_PARAM) return 0;
            }
            return 1;
        }

        default:
            return 0;
    }
}

/* ================================================================
 * Phase 1: Replace constants via RAUW
 *
 * For each SSA value whose lattice is CONST, replace all uses
 * with the known constant. Skip phi nodes (handled later by DCE
 * after branch folding) and values that are already constants.
 * ================================================================ */

static int replace_constants(SccpCtx *ctx, KlrFunc *fn)
{
    int changed = 0;
    KlrBasicBlock *bb;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn, *nxt_insn;
        insn_foreach_safe(insn, nxt_insn, bb) {
            /* Skip phi nodes — they'll be cleaned up by DCE */
            if (klr_is_phi((KlrValue *)insn)) continue;

            /* Skip terminators — branch folding handles them */
            if (insn_is_terminator(insn)) continue;

            /* Check if this instruction has a known constant lattice value */
            LatticeValue lv = sccp_read_lattice(ctx, (KlrValue *)insn);
            if (lv.tag != LATTICE_CONST) continue;

            /* Skip if the result is already the constant (no-op) */
            if (klr_is_const((KlrValue *)insn)) continue;

            /* Only replace instruction results */
            if (((KlrValue *)insn)->kind != KLR_VALUE_INSN) continue;

            log_info("[sccp-rewrite] replace const:");
            log_insn(insn);

            /* RAUW: replace all uses of this insn with the constant */
            if (replace_all_uses_with(lv.val, (KlrValue *)insn)) {
                changed = 1;
            }
        }
    }

    return changed;
}

/* ================================================================
 * Phase 2: Fold conditional branches
 *
 * If a conditional branch has a known-constant condition, fold it
 * to an unconditional jump to the taken branch.
 * ================================================================ */

static int fold_branches(SccpCtx *ctx, KlrFunc *fn)
{
    int changed = 0;
    KlrBasicBlock *bb;

    basic_block_foreach(bb, fn) {
        KlrInsn *insn = insn_last(bb);
        if (!insn || insn->code != OP_IR_JMP_COND) continue;

        /* Read lattice of the condition operand */
        LatticeValue lv = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
        if (lv.tag != LATTICE_CONST) continue;

        KlrConst *cc = (KlrConst *)lv.val;
        if (cc->which != CONST_BOOL) continue;

        int taken = cc->bval;
        /* Operand 2 = then block, operand 3 = else block */
        KlrBasicBlock *dst = (KlrBasicBlock *)insn_oper_value(insn, taken ? 2 : 3);
        ASSERT(dst->kind == KLR_VALUE_BLOCK);

        log_info("[sccp-rewrite] fold branch in '%%%s' -> '%%%s' (cond=%d)", klr_block_name(bb),
                 klr_block_name(dst), taken);

        /* Remove all outgoing edges and the conditional jump */
        klr_remove_all_out_edges(bb);
        klr_erase_insn(insn);

        /* Add unconditional jump to the target */
        KlrBuilder bldr;
        klr_builder_end(&bldr, bb);
        klr_build_jmp(&bldr, dst);

        changed = 1;
    }

    return changed;
}

/* ================================================================
 * Phase 3: Remove unreachable blocks
 *
 * BFS from the entry block (via sbb) to mark reachable blocks,
 * then erase any unvisited blocks.
 * ================================================================ */

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

/* ================================================================
 * Phase 4: Dead instruction elimination
 *
 * After constant replacement, many instructions have no remaining
 * uses. Remove them in a cascading fashion (removing one may make
 * its operands dead too).
 * ================================================================ */

static int eliminate_dead_code(KlrFunc *fn)
{
    int changed = 0;
    QUEUE(wklist);

    /* Seed worklist with dead, side-effect-free instructions */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (!klr_is_used((KlrValue *)insn) && !has_side_effect(insn)) {
                queue_push(&wklist, insn);
            }
        }
    }

    while (!queue_empty(&wklist)) {
        KlrInsn *insn = queue_pop(&wklist);

        /* Re-check: may have been erased or status changed */
        if (((KlrValue *)insn)->use_count > 0 || has_side_effect(insn)) {
            continue;
        }

        /* Check if any operands become dead after this removal */
        KlrValue *val;
        insn_oper_value_foreach(val, insn) {
            if (val->kind == KLR_VALUE_INSN && val->use_count == 1) {
                queue_push(&wklist, (KlrInsn *)val);
            }
        }

        log_info("[sccp-rewrite] erase dead insn:");
        log_insn(insn);

        klr_erase_insn(insn);
        changed = 1;
    }

    return changed;
}

/* ================================================================
 * SCCP Rewrite Pass Entry Point
 * ================================================================ */

int klr_sccp_rewrite_pass(KlrFunc *fn, void *data)
{
    (void)data;

    /* Skip trivial functions */
    if (!fn->sbb || list_empty(&fn->bb_list)) return 0;

    log_info("[sccp-rewrite] on func '%%%s'", fn->name);

    /* Step 0: Run SCCP analysis */
    SccpCtx *ctx = klr_sccp_analyze(fn);
    if (!ctx) return 0;

    int changed = 0;

    /* Step 1: Replace constants (RAUW) */
    changed |= replace_constants(ctx, fn);

    /* Step 2: Fold conditional branches */
    changed |= fold_branches(ctx, fn);

    /* Analysis context no longer needed after branch folding */
    sccp_ctx_destroy(ctx);

    /* Step 3: Remove unreachable blocks */
    changed |= remove_unreachable_blocks(fn);

    /* Step 4: Eliminate dead instructions (cascading) */
    changed |= eliminate_dead_code(fn);

    /* Step 5: Rebuild RPO after CFG modifications */
    if (changed) {
        klr_build_rpo(fn);
    }

    return changed;
}

#ifdef __cplusplus
}
#endif
/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SCCP_H_
#define _KOALA_SCCP_H_

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lattice tags and values (shared between sccp.c and sccp_rewrite.c) */
typedef enum {
    LATTICE_TOP,
    LATTICE_CONST,
    LATTICE_BOTTOM,
} LatticeTag;

typedef struct {
    LatticeTag tag;
    KlrValue *val; /* non-NULL only when tag == LATTICE_CONST */
} LatticeValue;

/* Opaque context type (forward declaration) */
typedef struct _SccpCtx SccpCtx;

/* Run analysis and return context (caller must call sccp_ctx_destroy) */
SccpCtx *klr_sccp_analyze(KlrFunc *fn);

/* Query functions */
LatticeValue sccp_read_lattice(SccpCtx *ctx, KlrValue *val);
int sccp_is_edge_exec(SccpCtx *ctx, KlrEdge *edge);
int sccp_is_reachable(SccpCtx *ctx, KlrBasicBlock *bb);

/* Destroy context */
void sccp_ctx_destroy(SccpCtx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SCCP_H_ */
/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/**
 * SCCP - Sparse Conditional Constant Propagation (Analysis Only)
 *
 * Based on Wegman & Zadeck, "Constant Propagation with Conditional Branches",
 * ACM TOPLAS 1991.
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
 *   CFG worklist - edges (KlrEdge*) to process for reachability
 *   SSA worklist - values (KlrValue*) whose lattice descended
 *
 * This pass is ANALYSIS ONLY. It computes lattice values but does NOT
 * rewrite the IR. A subsequent transform pass can query the lattice
 * to perform actual constant replacement.
 */

#include <math.h>
#include "log.h"
#include "opt.h"
#include "queue.h"
#include "sccp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * Forward declarations
 * ================================================================ */

static int consts_equal(KlrValue *a, KlrValue *b);

static inline LatticeValue lv_top(void) { return (LatticeValue){ LATTICE_TOP, NULL }; }

static inline LatticeValue lv_const(KlrValue *v) { return (LatticeValue){ LATTICE_CONST, v }; }

static inline LatticeValue lv_bottom(void) { return (LatticeValue){ LATTICE_BOTTOM, NULL }; }

/**
 * Lattice meet operation.
 * Result is written into *dst. Returns 1 if dst actually changed
 * (i.e., descended in the lattice).
 */
static int lv_meet(LatticeValue *dst, LatticeValue src)
{
    /* TOP meet x = x */
    if (dst->tag == LATTICE_TOP) {
        *dst = src;
        return src.tag != LATTICE_TOP;
    }
    /* BOTTOM absorbs everything */
    if (dst->tag == LATTICE_BOTTOM) return 0;
    /* dst is CONST */
    if (src.tag == LATTICE_TOP) return 0;
    if (src.tag == LATTICE_BOTTOM) {
        *dst = lv_bottom();
        return 1;
    }
    /* Both CONST: same value -> no change, different -> BOTTOM */
    if (consts_equal(dst->val, src.val)) return 0;
    *dst = lv_bottom();
    return 1;
}

/* ================================================================
 * HashMap Entries
 * ================================================================ */

/* Lattice map entry: KlrValue* -> LatticeValue */
typedef struct {
    HashMapEntry hnode;
    KlrValue *key;
    LatticeValue lv;
} LatticeEntry;

/* Executable edge set entry: KlrEdge* -> bool */
typedef struct {
    HashMapEntry hnode;
    KlrEdge *edge;
} EdgeFlagEntry;

/* ================================================================
 * SCCP Analysis Context
 * ================================================================ */

struct _SccpCtx {
    KlrFunc *func;
    HashMap lattice;   /* KlrValue* -> LatticeEntry */
    HashMap exe_edges; /* KlrEdge* -> EdgeFlagEntry (set) */
    Queue cfg_wl;      /* edges to process */
    Queue ssa_wl;      /* values to re-evaluate */
};

/* ================================================================
 * Lattice Map Helpers
 * ================================================================ */

static int lat_equal(void *a, void *b)
{
    return ((LatticeEntry *)a)->key == ((LatticeEntry *)b)->key;
}

static LatticeValue *get_lattice(SccpCtx *ctx, KlrValue *val)
{
    LatticeEntry probe = { .key = val };
    hashmap_entry_init(&probe.hnode, mem_hash(&val, sizeof(val)));
    LatticeEntry *e = hashmap_get(&ctx->lattice, &probe);
    return e ? &e->lv : NULL;
}

/** Read lattice value; returns TOP if no entry exists. */
LatticeValue sccp_read_lattice(SccpCtx *ctx, KlrValue *val)
{
    /* IR constants are always CONST, no need for map lookup */
    if (klr_is_const(val)) return lv_const(val);

    LatticeValue *lv = get_lattice(ctx, val);
    return lv ? *lv : lv_top();
}

/**
 * Set lattice value. Returns 1 if the value actually descended.
 * Creates the entry on first access (initial value = TOP).
 */
static int set_lattice(SccpCtx *ctx, KlrValue *val, LatticeValue new_lv)
{
    LatticeEntry probe = { .key = val };
    hashmap_entry_init(&probe.hnode, mem_hash(&val, sizeof(val)));

    LatticeEntry *e = hashmap_get(&ctx->lattice, &probe);
    if (!e) {
        /* First access: old value is implicitly TOP */
        if (new_lv.tag == LATTICE_TOP) return 0;
        e = mm_alloc_obj(e);
        hashmap_entry_init(&e->hnode, mem_hash(&val, sizeof(val)));
        e->key = val;
        e->lv = new_lv;
        hashmap_put_only(&ctx->lattice, &e->hnode);
        return 1;
    }
    return lv_meet(&e->lv, new_lv);
}

/* ================================================================
 * Executable Edge Helpers
 * ================================================================ */

static int edge_equal(void *a, void *b)
{
    return ((EdgeFlagEntry *)a)->edge == ((EdgeFlagEntry *)b)->edge;
}

/** Mark edge as executable. Returns 1 if it was NOT already marked. */
static int mark_edge_exec(SccpCtx *ctx, KlrEdge *edge)
{
    EdgeFlagEntry probe = { .edge = edge };
    hashmap_entry_init(&probe.hnode, mem_hash(&edge, sizeof(edge)));

    if (hashmap_get(&ctx->exe_edges, &probe)) return 0;

    EdgeFlagEntry *e = mm_alloc_obj(e);
    hashmap_entry_init(&e->hnode, mem_hash(&edge, sizeof(edge)));
    e->edge = edge;
    hashmap_put_only(&ctx->exe_edges, &e->hnode);
    return 1;
}

int sccp_is_edge_exec(SccpCtx *ctx, KlrEdge *edge)
{
    EdgeFlagEntry probe = { .edge = edge };
    hashmap_entry_init(&probe.hnode, mem_hash(&edge, sizeof(edge)));
    return hashmap_get(&ctx->exe_edges, &probe) != NULL;
}

/* ================================================================
 * Block Reachability (uses bb->visited flag)
 * ================================================================ */

int sccp_is_reachable(SccpCtx *ctx, KlrBasicBlock *bb)
{
    (void)ctx;
    return bb->visited;
}

static inline void mark_reachable(KlrBasicBlock *bb) { bb->visited = 1; }

/* ================================================================
 * Edge Lookup
 * ================================================================ */

static KlrEdge *find_edge(SccpCtx *ctx, KlrBasicBlock *src, KlrBasicBlock *dst)
{
    (void)ctx;
    KlrEdge *edge;
    edge_out_foreach(edge, src) {
        if (edge->dst == dst) return edge;
    }
    return NULL;
}

/* ================================================================
 * Push Users to SSA Worklist
 *
 * When a value's lattice descends, all instructions using it need
 * re-evaluation. For phi users, we only push if the edge from the
 * phi's predecessor block is executable (non-executable operands
 * don't contribute to the phi result).
 * ================================================================ */

static void push_users(SccpCtx *ctx, KlrValue *val)
{
    KlrUse *use;
    use_foreach(use, val) {
        if (use->is_def) continue;
        KlrInsn *user = use->insn;
        if (!user) continue;

        if (klr_is_phi((KlrValue *)user)) {
            /* Find the phi operand index for this use, check edge */
            for (int i = 0; i < user->filled; i++) {
                if (insn_oper_value(user, i) == val) {
                    KlrBasicBlock *pred = user->phi_preds[i];
                    KlrEdge *e = find_edge(ctx, pred, user->bb);
                    if (e && sccp_is_edge_exec(ctx, e)) {
                        queue_push(&ctx->ssa_wl, user);
                    }
                    break;
                }
            }
        } else {
            queue_push(&ctx->ssa_wl, user);
        }
    }
}

/* ================================================================
 * Constant Equality Check
 * ================================================================ */

static int consts_equal(KlrValue *a, KlrValue *b)
{
    if (a == b) return 1;
    if (!klr_is_const(a) || !klr_is_const(b)) return 0;

    KlrConst *ca = (KlrConst *)a;
    KlrConst *cb = (KlrConst *)b;
    if (ca->which != cb->which) return 0;

    switch (ca->which) {
        case CONST_INT:
        case CONST_UINT:
            return ca->ival == cb->ival;
        case CONST_FLT:
            return ca->fval == cb->fval;
        case CONST_BOOL:
            return ca->bval == cb->bval;
        case CONST_NONE:
            return 1;
        case CONST_STR:
            return (ca->len == cb->len) && !strcmp(ca->sval, cb->sval);
        default:
            return 0;
    }
}

/* ================================================================
 * Binary Operation Constant Folding
 *
 * Returns LATTICE_CONST if both operands are foldable constants,
 * LATTICE_BOTTOM if either is BOTTOM, LATTICE_TOP otherwise.
 * ================================================================ */

static LatticeValue fold_binary(SccpCtx *ctx, KlrInsn *insn, LatticeValue lhs, LatticeValue rhs)
{
    if (lhs.tag == LATTICE_BOTTOM || rhs.tag == LATTICE_BOTTOM) return lv_bottom();
    if (lhs.tag == LATTICE_TOP || rhs.tag == LATTICE_TOP) return lv_top();

    /* Both CONST - attempt to fold */
    KlrConst *lc = (KlrConst *)lhs.val;
    KlrConst *rc = (KlrConst *)rhs.val;
    KlrModule *m = ctx->func->module;

    /* Integer arithmetic (signed and unsigned share bit patterns) */
    if ((lc->which == CONST_INT || lc->which == CONST_UINT) &&
        (rc->which == CONST_INT || rc->which == CONST_UINT)) {
        int64_t li = (int64_t)lc->ival, ri = (int64_t)rc->ival;
        uint64_t lu = lc->ival, ru = rc->ival;
        int64_t result;

        switch (insn->code) {
            case OP_BINARY_ADD:
                result = li + ri;
                break;
            case OP_BINARY_SUB:
                result = li - ri;
                break;
            case OP_BINARY_MUL:
                result = li * ri;
                break;
            case OP_BINARY_DIV:
                if (ri == 0) return lv_bottom();
                result = li / ri;
                break;
            case OP_BINARY_MOD:
                if (ri == 0) return lv_bottom();
                result = li % ri;
                break;
            case OP_BINARY_AND:
                result = li & ri;
                break;
            case OP_BINARY_OR:
                result = li | ri;
                break;
            case OP_BINARY_XOR:
                result = li ^ ri;
                break;
            case OP_BINARY_SHL:
                result = li << (ri & 63);
                break;
            case OP_BINARY_SHR:
                result = li >> (ri & 63);
                break;

            /* Comparisons -> bool */
            case OP_BINARY_CMPEQ:
                return lv_const(klr_const_bool(lu == ru, m));
            case OP_BINARY_CMPNE:
                return lv_const(klr_const_bool(lu != ru, m));
            case OP_BINARY_CMPLT:
                return lv_const(klr_const_bool(li < ri, m));
            case OP_BINARY_CMPLE:
                return lv_const(klr_const_bool(li <= ri, m));
            case OP_BINARY_CMPGT:
                return lv_const(klr_const_bool(li > ri, m));
            case OP_BINARY_CMPGE:
                return lv_const(klr_const_bool(li >= ri, m));

            default:
                return lv_bottom();
        }
        if (lc->which == CONST_UINT && rc->which == CONST_UINT)
            return lv_const(klr_const_uint((uint64_t)result, insn->ts, m));
        return lv_const(klr_const_int(result, insn->ts, m));
    }

    /* Float arithmetic */
    if (lc->which == CONST_FLT && rc->which == CONST_FLT) {
        double l = lc->fval, r = rc->fval;
        double res;
        switch (insn->code) {
            case OP_BINARY_ADD:
                res = l + r;
                break;
            case OP_BINARY_SUB:
                res = l - r;
                break;
            case OP_BINARY_MUL:
                res = l * r;
                break;
            case OP_BINARY_DIV:
                if (r == 0.0) return lv_bottom();
                res = l / r;
                break;
            case OP_BINARY_MOD:
                if (r == 0.0) return lv_bottom();
                res = fmod(l, r);
                break;
            case OP_BINARY_CMPEQ:
                return lv_const(klr_const_bool(l == r, m));
            case OP_BINARY_CMPNE:
                return lv_const(klr_const_bool(l != r, m));
            case OP_BINARY_CMPLT:
                return lv_const(klr_const_bool(l < r, m));
            case OP_BINARY_CMPLE:
                return lv_const(klr_const_bool(l <= r, m));
            case OP_BINARY_CMPGT:
                return lv_const(klr_const_bool(l > r, m));
            case OP_BINARY_CMPGE:
                return lv_const(klr_const_bool(l >= r, m));
            default:
                return lv_bottom();
        }
        return lv_const(klr_const_float(res, insn->ts, m));
    }

    /* Unsupported constant combination */
    return lv_bottom();
}

/* ================================================================
 * Instruction Evaluation
 *
 * Compute the lattice value produced by an instruction based on
 * current operand lattice values. Does NOT modify the lattice map.
 * ================================================================ */

static LatticeValue evaluate_insn(SccpCtx *ctx, KlrInsn *insn)
{
    /* ---- Phi: meet of operands from executable predecessors ---- */
    if (insn->code == OP_IR_PHI) {
        LatticeValue result = lv_top();
        int has_exec_pred = 0;

        for (int i = 0; i < insn->filled; i++) {
            KlrBasicBlock *pred = insn->phi_preds[i];
            KlrEdge *e = find_edge(ctx, pred, insn->bb);
            if (e && sccp_is_edge_exec(ctx, e)) {
                has_exec_pred = 1;
                LatticeValue oper_lv = sccp_read_lattice(ctx, insn_oper_value(insn, i));
                lv_meet(&result, oper_lv);
                if (result.tag == LATTICE_BOTTOM) break;
            }
        }

        if (!has_exec_pred) return lv_top();
        return result;
    }

    /* ---- Binary arithmetic / comparison ---- */
    if (insn->code >= OP_BINARY_ADD && insn->code <= OP_BINARY_CMPGE) {
        LatticeValue lhs = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
        LatticeValue rhs = sccp_read_lattice(ctx, insn_oper_value(insn, 1));
        return fold_binary(ctx, insn, lhs, rhs);
    }

    /* ---- Unary operations ---- */
    if (insn->code >= OP_UNARY_PLUS && insn->code <= OP_UNARY_NOT) {
        LatticeValue operand = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
        if (operand.tag == LATTICE_BOTTOM) return lv_bottom();
        if (operand.tag == LATTICE_TOP) return lv_top();

        KlrConst *c = (KlrConst *)operand.val;
        KlrModule *m = ctx->func->module;

        switch (insn->code) {
            case OP_UNARY_PLUS:
                return operand;
            case OP_UNARY_NEG:
                if (c->which == CONST_INT)
                    return lv_const(klr_const_int(-(int64_t)c->ival, insn->ts, m));
                if (c->which == CONST_FLT) return lv_const(klr_const_float(-c->fval, insn->ts, m));
                return lv_bottom();
            case OP_UNARY_NOT:
                if (c->which == CONST_BOOL) return lv_const(klr_const_bool(!c->bval, m));
                return lv_bottom();
            default:
                return lv_bottom();
        }
    }

    /* ---- Select: cond ? true_val : false_val ---- */
    if (insn->code == OP_IR_SELECT) {
        LatticeValue cond = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
        if (cond.tag == LATTICE_BOTTOM) return lv_bottom();
        if (cond.tag == LATTICE_CONST) {
            KlrConst *cc = (KlrConst *)cond.val;
            if (cc->which == CONST_BOOL) {
                return cc->bval ? sccp_read_lattice(ctx, insn_oper_value(insn, 1))
                                : sccp_read_lattice(ctx, insn_oper_value(insn, 2));
            }
            return lv_bottom();
        }
        /* cond is TOP: return TOP, do not speculatively descend */
        return lv_top();
    }

    /* ---- Cast: try constant folding ---- */
    if (insn->code == OP_IR_CAST) {
        LatticeValue operand = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
        if (operand.tag == LATTICE_BOTTOM) return lv_bottom();
        if (operand.tag == LATTICE_TOP) return lv_top();
        /* Constant folding for casts is complex (width changes, overflow
         * checks); conservatively return BOTTOM for analysis. */
        return lv_bottom();
    }

    /* ---- Calls, loads, and everything else: conservative BOTTOM ---- */
    return lv_bottom();
}

/* ================================================================
 * CFG Edge Processing
 *
 * Per Wegman & Zadeck: the CFG worklist stores control-flow edges.
 * Processing an edge (P->S) marks it executable and updates S's
 * phi nodes and reachability.
 * ================================================================ */

static void process_cfg_edge(SccpCtx *ctx, KlrEdge *edge)
{
    /* Skip if already marked executable */
    if (!mark_edge_exec(ctx, edge)) return;

    KlrBasicBlock *p = edge->src;
    KlrBasicBlock *s = edge->dst;
    (void)p;

    if (sccp_is_reachable(ctx, s)) {
        /*
         * S already reachable: this is a new executable edge to a
         * previously-visited block. Only phi operands from P need to
         * be incrementally met into S's phi accumulators.
         */
        KlrInsn *insn;
        insn_foreach(insn, s) {
            if (!klr_is_phi((KlrValue *)insn)) break;

            LatticeValue old_lv = sccp_read_lattice(ctx, (KlrValue *)insn);
            if (old_lv.tag == LATTICE_BOTTOM) continue;

            /* Find this edge's phi operand */
            for (int i = 0; i < insn->filled; i++) {
                if (insn->phi_preds[i] == p) {
                    LatticeValue oper_lv = sccp_read_lattice(ctx, insn_oper_value(insn, i));
                    if (set_lattice(ctx, (KlrValue *)insn, oper_lv)) {
                        push_users(ctx, (KlrValue *)insn);
                    }
                    break;
                }
            }
        }
    } else {
        /*
         * S becomes reachable for the first time. Evaluate all phis,
         * then all non-phi instructions, then the terminator.
         */
        mark_reachable(s);
        log_info("[sccp] block %%bb%d becomes reachable", s->tag);

        /* Evaluate phis */
        KlrInsn *insn;
        insn_foreach(insn, s) {
            if (!klr_is_phi((KlrValue *)insn)) break;
            LatticeValue lv = evaluate_insn(ctx, insn);
            if (lv.tag != LATTICE_TOP) {
                set_lattice(ctx, (KlrValue *)insn, lv);
            }
        }

        /* Evaluate non-phi instructions */
        int past_phi = 0;
        insn_foreach(insn, s) {
            if (!past_phi && !klr_is_phi((KlrValue *)insn)) past_phi = 1;
            if (!past_phi) continue;
            if (insn_is_terminator(insn)) break;

            LatticeValue lv = evaluate_insn(ctx, insn);
            if (lv.tag != LATTICE_TOP) {
                if (set_lattice(ctx, (KlrValue *)insn, lv)) {
                    push_users(ctx, (KlrValue *)insn);
                }
            }
        }

        /* Evaluate terminator -> push executable successor edges */
        insn = insn_last(s);
        if (insn && insn_is_terminator(insn)) {
            switch (insn->code) {
                case OP_IR_JMP_COND: {
                    LatticeValue cond = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
                    KlrBasicBlock *then_bb = (KlrBasicBlock *)insn_oper_value(insn, 2);
                    KlrBasicBlock *else_bb = (KlrBasicBlock *)insn_oper_value(insn, 3);

                    if (cond.tag == LATTICE_CONST) {
                        KlrConst *cc = (KlrConst *)cond.val;
                        KlrBasicBlock *taken =
                            (cc->which == CONST_BOOL && cc->bval) ? then_bb : else_bb;
                        KlrEdge *e = find_edge(ctx, s, taken);
                        if (e) queue_push(&ctx->cfg_wl, e);
                    } else if (cond.tag == LATTICE_BOTTOM) {
                        /* Both branches potentially taken */
                        KlrEdge *e;
                        edge_out_foreach(e, s) {
                            queue_push(&ctx->cfg_wl, e);
                        }
                    }
                    /* TOP: no edges yet */
                    break;
                }

                case OP_JMP:
                case OP_RET:
                case OP_RET_VOID: {
                    /* Unconditional: all outgoing edges are executable */
                    KlrEdge *e;
                    edge_out_foreach(e, s) {
                        queue_push(&ctx->cfg_wl, e);
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
}

/* ================================================================
 * SSA Worklist Processing
 *
 * A value was pushed because one of its operands descended.
 * Re-evaluate the defining instruction and propagate changes.
 * ================================================================ */

static void process_ssa_value(SccpCtx *ctx, KlrValue *val)
{
    if (!klr_is_insn(val)) return;

    KlrInsn *insn = (KlrInsn *)val;
    if (insn_is_terminator(insn)) {
        /* Re-evaluate terminator: may discover new executable edges
         * when the branch condition descends from TOP to CONST/BOTTOM. */
        KlrBasicBlock *bb = insn->bb;
        if (!sccp_is_reachable(ctx, bb)) return;

        if (insn->code == OP_IR_JMP_COND) {
            LatticeValue cond = sccp_read_lattice(ctx, insn_oper_value(insn, 0));
            KlrBasicBlock *then_bb = (KlrBasicBlock *)insn_oper_value(insn, 2);
            KlrBasicBlock *else_bb = (KlrBasicBlock *)insn_oper_value(insn, 3);

            if (cond.tag == LATTICE_CONST) {
                KlrConst *cc = (KlrConst *)cond.val;
                KlrBasicBlock *taken = (cc->which == CONST_BOOL && cc->bval) ? then_bb : else_bb;
                KlrEdge *e = find_edge(ctx, bb, taken);
                if (e) queue_push(&ctx->cfg_wl, e);
            } else if (cond.tag == LATTICE_BOTTOM) {
                KlrEdge *e;
                edge_out_foreach(e, bb) {
                    queue_push(&ctx->cfg_wl, e);
                }
            }
        }
        return;
    }

    /* Re-evaluate instruction */
    LatticeValue new_lv = evaluate_insn(ctx, insn);
    if (set_lattice(ctx, val, new_lv)) {
        push_users(ctx, val);
    }
}

/* ================================================================
 * Context Init / Cleanup
 * ================================================================ */

static void sccp_init(SccpCtx *ctx, KlrFunc *fn)
{
    ctx->func = fn;
    hashmap_init(&ctx->lattice, lat_equal);
    hashmap_init(&ctx->exe_edges, edge_equal);
    init_queue(&ctx->cfg_wl);
    init_queue(&ctx->ssa_wl);
}

static void lat_free(void *entry, void *arg)
{
    (void)arg;
    mm_free(entry);
}

static void edge_free(void *entry, void *arg)
{
    (void)arg;
    mm_free(entry);
}

static void sccp_fini(SccpCtx *ctx)
{
    hashmap_fini(&ctx->lattice, lat_free, NULL);
    hashmap_fini(&ctx->exe_edges, edge_free, NULL);

    /* Clear visited flags set during analysis */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, ctx->func) {
        bb->visited = 0;
    }
}

/* ================================================================
 * Public API: Analysis, Query, Destroy
 * ================================================================ */

SccpCtx *klr_sccp_analyze(KlrFunc *fn)
{
    /* Skip trivial functions */
    if (!fn->sbb || list_empty(&fn->bb_list)) return NULL;

    log_info("[sccp] analysis on func '%%%s'", fn->name);

    SccpCtx *ctx = mm_alloc(sizeof(SccpCtx));
    sccp_init(ctx, fn);

    /* Clear visited flags from any previous pass */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        bb->visited = 0;
    }

    /*
     * Seed the analysis: push the edge from the start-block sentinel
     * into the CFG worklist. This marks the real entry block reachable.
     */
    KlrEdge *entry_edge = edge_out_first(fn->sbb);
    if (entry_edge) {
        queue_push(&ctx->cfg_wl, entry_edge);
    }

    /* Main analysis loop: alternate between CFG and SSA worklists */
    while (!queue_empty(&ctx->cfg_wl) || !queue_empty(&ctx->ssa_wl)) {
        while (!queue_empty(&ctx->cfg_wl)) {
            KlrEdge *edge = queue_pop(&ctx->cfg_wl);
            process_cfg_edge(ctx, edge);
        }
        while (!queue_empty(&ctx->ssa_wl)) {
            KlrValue *val = queue_pop(&ctx->ssa_wl);
            process_ssa_value(ctx, val);
        }
    }

    /* Analysis complete - lattice values are available for querying. */
    return ctx;
}

void sccp_ctx_destroy(SccpCtx *ctx)
{
    if (!ctx) return;
    sccp_fini(ctx);
    mm_free(ctx);
}

/* ================================================================
 * Pass Entry Point (analysis-only, rewrite is in sccp_rewrite.c)
 * ================================================================ */

int klr_sccp_pass(KlrFunc *fn, void *data)
{
    (void)data;
    SccpCtx *ctx = klr_sccp_analyze(fn);
    if (!ctx) return 0;
    /* Analysis only for now — rewrite is in separate pass */
    sccp_ctx_destroy(ctx);
    return 0;
}

#ifdef __cplusplus
}
