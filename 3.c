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

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * Lattice Value
 * ================================================================ */

typedef enum {
    LATTICE_TOP,
    LATTICE_CONST,
    LATTICE_BOTTOM,
} LatticeTag;

typedef struct {
    LatticeTag tag;
    KlrValue *val; /* non-NULL only when tag == LATTICE_CONST */
} LatticeValue;

/* Forward declarations */
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

typedef struct {
    KlrFunc *func;
    HashMap lattice;   /* KlrValue* -> LatticeEntry */
    HashMap exe_edges; /* KlrEdge* -> EdgeFlagEntry (set) */
    Queue cfg_wl;      /* edges to process */
    Queue ssa_wl;      /* values to re-evaluate */
} SccpCtx;

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
static LatticeValue read_lattice(SccpCtx *ctx, KlrValue *val)
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

static int is_edge_exec(SccpCtx *ctx, KlrEdge *edge)
{
    EdgeFlagEntry probe = { .edge = edge };
    hashmap_entry_init(&probe.hnode, mem_hash(&edge, sizeof(edge)));
    return hashmap_get(&ctx->exe_edges, &probe) != NULL;
}

/* ================================================================
 * Block Reachability (uses bb->visited flag)
 * ================================================================ */

static inline int is_reachable(KlrBasicBlock *bb) { return bb->visited; }
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
                    if (e && is_edge_exec(ctx, e)) {
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
            if (e && is_edge_exec(ctx, e)) {
                has_exec_pred = 1;
                LatticeValue oper_lv = read_lattice(ctx, insn_oper_value(insn, i));
                lv_meet(&result, oper_lv);
                if (result.tag == LATTICE_BOTTOM) break;
            }
        }

        if (!has_exec_pred) return lv_top();
        return result;
    }

    /* ---- Binary arithmetic / comparison ---- */
    if (insn->code >= OP_BINARY_ADD && insn->code <= OP_BINARY_CMPGE) {
        LatticeValue lhs = read_lattice(ctx, insn_oper_value(insn, 0));
        LatticeValue rhs = read_lattice(ctx, insn_oper_value(insn, 1));
        return fold_binary(ctx, insn, lhs, rhs);
    }

    /* ---- Unary operations ---- */
    if (insn->code >= OP_UNARY_PLUS && insn->code <= OP_UNARY_NOT) {
        LatticeValue operand = read_lattice(ctx, insn_oper_value(insn, 0));
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
        LatticeValue cond = read_lattice(ctx, insn_oper_value(insn, 0));
        if (cond.tag == LATTICE_BOTTOM) return lv_bottom();
        if (cond.tag == LATTICE_CONST) {
            KlrConst *cc = (KlrConst *)cond.val;
            if (cc->which == CONST_BOOL) {
                return cc->bval ? read_lattice(ctx, insn_oper_value(insn, 1))
                                : read_lattice(ctx, insn_oper_value(insn, 2));
            }
            return lv_bottom();
        }
        /* cond is TOP: return TOP, do not speculatively descend */
        return lv_top();
    }

    /* ---- Cast: try constant folding ---- */
    if (insn->code == OP_IR_CAST) {
        LatticeValue operand = read_lattice(ctx, insn_oper_value(insn, 0));
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

    if (is_reachable(s)) {
        /*
         * S already reachable: this is a new executable edge to a
         * previously-visited block. Only phi operands from P need to
         * be incrementally met into S's phi accumulators.
         */
        KlrInsn *insn;
        insn_foreach(insn, s) {
            if (!klr_is_phi((KlrValue *)insn)) break;

            LatticeValue old_lv = read_lattice(ctx, (KlrValue *)insn);
            if (old_lv.tag == LATTICE_BOTTOM) continue;

            /* Find this edge's phi operand */
            for (int i = 0; i < insn->filled; i++) {
                if (insn->phi_preds[i] == p) {
                    LatticeValue oper_lv = read_lattice(ctx, insn_oper_value(insn, i));
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
                    LatticeValue cond = read_lattice(ctx, insn_oper_value(insn, 0));
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
        if (!is_reachable(bb)) return;

        if (insn->code == OP_IR_JMP_COND) {
            LatticeValue cond = read_lattice(ctx, insn_oper_value(insn, 0));
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
 * Pass Entry Point
 * ================================================================ */

int klr_sccp_pass(KlrFunc *fn, void *data)
{
    (void)data;

    /* Skip trivial functions */
    if (!fn->sbb || list_empty(&fn->bb_list)) return 0;

    log_info("[sccp] analysis on func '%%%s'", fn->name);

    SccpCtx ctx;
    sccp_init(&ctx, fn);

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
        queue_push(&ctx.cfg_wl, entry_edge);
    }

    /* Main analysis loop: alternate between CFG and SSA worklists */
    while (!queue_empty(&ctx.cfg_wl) || !queue_empty(&ctx.ssa_wl)) {
        while (!queue_empty(&ctx.cfg_wl)) {
            KlrEdge *edge = queue_pop(&ctx.cfg_wl);
            process_cfg_edge(&ctx, edge);
        }
        while (!queue_empty(&ctx.ssa_wl)) {
            KlrValue *val = queue_pop(&ctx.ssa_wl);
            process_ssa_value(&ctx, val);
        }
    }

    /* Analysis complete - lattice values are available for querying.
     * This pass does NOT modify the IR. */
    sccp_fini(&ctx);

    return 0;
}

#ifdef __cplusplus
}
#endif
