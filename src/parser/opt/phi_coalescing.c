/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "log.h"
#include "opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlrCoalesceContext {
    /* Tracker of all allocated KlrCongruenceValue nodes */
    Vector values;
    /* Global member directory: Value* -> Member* */
    HashMap member_map;
    /* all unique, final converged KlrCongruenceClass */
    Vector final_classes;
} KlrCoalesceContext;

// Congruence Value
// Binds a raw SSA value with precise live-range time-slice coordinates
typedef struct _KlrCongruenceValue {
    /* The raw SSA variable pointer */
    KlrValue *val;
    /* Left-closed program point position */
    int start;
    /* Right-open boundary program point position */
    int end;
} KlrCongruenceValue;

// Congruence Class
// Represents a set of congruent SSA values
typedef struct _KlrCongruenceClass {
    /* representative leader of this class */
    KlrCongruenceValue *repr;
    /* containing pointers to KlrCongruenceMember */
    Vector members;
} KlrCongruenceClass;

// Congruence Member
// Flattened member entity mapping a single value back to its owner class
typedef struct _KlrCongruenceMember {
    HashMapEntry hnode;
    KlrCongruenceValue *key; // SSA Value
    KlrCongruenceClass *cls; // Congruence class
} KlrCongruenceMember;

typedef struct _PhiEdge {
    KlrBasicBlock *pred;
    KlrValue *src;
} PhiEdge;

static void _assign_coord(KlrFunc *fn)
{
    int pos = 1; // Position 0 is reserved for global incoming parameters
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            insn->pos = pos++;
        }
        /* Open-ended basic block boundary: bb->last_pos acts as the right-open cut point */
        bb->last_pos = pos;
    }
}

/* Compute interval end as max of all use points in [start, end) model.
 * PHI operand: consumed at pred->last_pos (predecessor exit).
 * Normal use:  consumed at insn->pos.
 */
static int _build_interval_end(KlrValue *val)
{
    /* Constants have no uses — end is already set to 0 by caller */
    if (klr_is_const(val)) return 0;

    int end = -1;

    KlrUse *use;
    use_foreach(use, val) {
        int point;

        KlrInsn *insn = use->insn;

        if (insn->code == OP_IR_PHI) {
            int index = (KlrOper *)use - insn->opers;
            KlrBasicBlock *pred = insn->phi_preds[index];
            point = pred->last_pos; /* Directly references the open-ended boundary */
        } else {
            point = insn->pos;
        }

        // max(op_last_used_pos, pred_bb_pos)
        if (point > end) {
            end = point;
        }
    }

    return end;
}

/* Builds the initial raw boundaries for the congruence value */
static void _build_interval(KlrCongruenceValue *val)
{
    KlrValue *v = val->val;

    if (klr_is_param(v)) {
        val->start = 0;
    } else if (klr_is_const(v)) {
        val->start = 0; /* Constants are considered to have a start position of 0 */
    } else {
        ASSERT(klr_is_insn(v));
        val->start = ((KlrInsn *)v)->pos;
    }

    val->end = _build_interval_end(v);
}

static KlrCongruenceMember *_find_member(KlrCoalesceContext *ctx, KlrCongruenceValue *cong_val)
{
    KlrCongruenceMember key = { .key = cong_val, .cls = NULL };
    hashmap_entry_init(&key, mem_hash(&cong_val->val, sizeof(KlrValue *)));
    return hashmap_get(&ctx->member_map, &key);
}

static KlrCongruenceMember *_find_member_by_ssa(KlrCoalesceContext *ctx, KlrValue *val)
{
    KlrCongruenceValue dummy_val = { .val = val };
    return _find_member(ctx, &dummy_val);
}

static void _register_value(KlrValue *val, KlrCoalesceContext *ctx)
{
    if (_find_member_by_ssa(ctx, val)) {
        log_info("[COALESCE] Value already registered: %s", klr_value_name(val));
        return;
    }

    /* 1. Allocate the profile ledger */
    KlrCongruenceValue *cong_val = mm_alloc_obj(cong_val);
    cong_val->val = val;
    _build_interval(cong_val);
    vector_push_back(&ctx->values, &cong_val);

    /* 2. Allocate the empty singleton class container */
    KlrCongruenceClass *cls = mm_alloc_obj(cls);
    cls->repr = cong_val;
    vector_init_ptr(&cls->members);

    /* 3. Allocate the member directory mapping block */
    KlrCongruenceMember *member = mm_alloc_obj(member);
    hashmap_entry_init(member, mem_hash(&val, sizeof(KlrValue *)));
    member->key = cong_val;
    member->cls = cls;

    vector_push_back(&cls->members, &member);

    hashmap_put(&ctx->member_map, member);

    log_info("[COALESCE] Registered value: %s, interval: [%d, %d)", klr_value_name(val),
             cong_val->start, cong_val->end);
}

/* returns its canonical representative profile. */
static KlrCongruenceValue *_find_repr(KlrCoalesceContext *ctx, KlrValue *val)
{
    KlrCongruenceMember *member = _find_member_by_ssa(ctx, val);
    ASSERT(member && member->cls && member->cls->repr);
    return member->cls->repr;
}

/*
 * Checks if two left-closed, right-open ranges [start, end) overlap.
 * Returns true if ranges collide, returns false if they are perfectly disjoint.
 */
static bool _intervals_overlap(KlrCongruenceValue *a, KlrCongruenceValue *b)
{
    /* Pure positive logic: geometric overlap occurs if and only if both intersect */
    return a->start < b->end && b->start < a->end;
}

static bool _check_interference(KlrCoalesceContext *ctx, KlrCongruenceClass *dst,
                                KlrCongruenceClass *src)
{
    int dst_size = vector_size(&dst->members);
    int src_size = vector_size(&src->members);

    /* Match every single member from dst against every member in src */
    for (int i = 0; i < dst_size; i++) {
        KlrCongruenceMember *m_dst = vector_get(&dst->members, i);
        KlrCongruenceValue *intv_dst = m_dst->key;

        for (int j = 0; j < src_size; j++) {
            KlrCongruenceMember *m_src = vector_get(&src->members, j);
            KlrCongruenceValue *intv_src = m_src->key;

            if (_intervals_overlap(intv_dst, intv_src)) {
                return true;
            }
        }
    }
    return false;
}

static KlrCongruenceClass *_get_class(KlrCoalesceContext *ctx, KlrCongruenceValue *cong_val)
{
    KlrCongruenceMember *member = _find_member(ctx, cong_val);
    ASSERT(member && member->cls);
    return member->cls;
}

/* Elect Leader: Implements election heuristics (PHI Node Priority + Size Balance).
 * Evaluates both classes and orders them such that the optimal leader remains cls_dst,
 * significantly reducing dynamic vector allocation overhead across fixpoint runs.
 */
static void _elect_leader(KlrCongruenceClass **cls_dst_p, KlrCongruenceClass **cls_src_p)
{
    KlrCongruenceClass *cls_dst = *cls_dst_p;
    KlrCongruenceClass *cls_src = *cls_src_p;

    bool swap_needed = false;
    bool is_phi_dst = klr_is_phi(cls_dst->repr->val);
    bool is_phi_src = klr_is_phi(cls_src->repr->val);

    if (is_phi_dst != is_phi_src) {
        /* Force the PHI node instruction to always sit on the canonical throne */
        swap_needed = (!is_phi_dst && is_phi_src);
    } else {
        /* Heuristic: Merge the smaller vector roster into the larger vector roster */
        swap_needed = (vector_size(&cls_dst->members) < vector_size(&cls_src->members));
    }

    if (swap_needed) {
        *cls_dst_p = cls_src;
        *cls_src_p = cls_dst;
    }
}

/* Union Class: Transfers all members from src to dst and destroys src roster.
 * Leverages an overhead-free O(1) While-Pop strategy to enforce absolute memory safety.
 */
static void _union_class(KlrCongruenceClass *cls_dst, KlrCongruenceClass *cls_src)
{
    /* Pop elements from the back to preserve array memory boundaries and avoid shifts */
    KlrCongruenceMember *member;
    vector_foreach(member, &cls_src->members) {
        /* Re-map member's active ledger directly to the victor class: flat O(1) jump */
        member->cls = cls_dst;
        /* Insert the student seamlessly into the target destination class master roster */
        vector_push_back(&cls_dst->members, &member);
    }

    /* Teardown: Source vector length is 0. Safely clean up its container shell footprint */
    vector_fini(&cls_src->members);
}

static bool _try_merge(KlrCoalesceContext *ctx, KlrCongruenceValue *dst, KlrCongruenceValue *src)
{
    if (dst == src) return false;

    KlrCongruenceClass *cls_dst = _get_class(ctx, dst);
    KlrCongruenceClass *cls_src = _get_class(ctx, src);

    /* Direct double-pointer swap layout dynamically aligns the leader ranking */
    _elect_leader(&cls_dst, &cls_src);

    /* Terminate merging path immediately on true overlap interference */
    if (_check_interference(ctx, cls_dst, cls_src)) return false;

    /* Execution: combines ranges securely */
    _union_class(cls_dst, cls_src);

    return true;
}

/* Dump Congruence Classes: Exhaustively dumps all registered active congruence classes and
 * members. Relies on the unique leader-identity match to completely filter out duplicate
 * iterations, creating a clean, hierarchical topological layout for comprehensive validation
 * tracking.
 */
static void _dump_congruence_classes(KlrCoalesceContext *ctx, const char *phase_tag)
{
    log_info("[COALESCE] ========================================================");
    log_info("[COALESCE] >>> CONGRUENCE ROSTER SNAPSHOT [%s] <<<", phase_tag);
    log_info("[COALESCE] ========================================================");

    int class_counter = 0;

    HashMapIter it = { 0 };

    while (hashmap_next(&ctx->member_map, &it)) {
        /* Adjust the function call to match your actual iterator macro layout if required */
        KlrCongruenceMember *member = (KlrCongruenceMember *)it.entry;
        if (!member || !member->cls) {
            continue;
        }

        KlrCongruenceClass *cls = member->cls;

        /* Dedup Gate: Only dump the class when the traversed member profile acts as the supreme
         * Leader
         */
        if (member->key != cls->repr) {
            continue;
        }

        class_counter++;
        int class_size = vector_size(&cls->members);

        log_info(
            "[COALESCE] Class #%d [Leader repr: %s, Base LiveInterval: [%d, %d), Members: %d]",
            class_counter, klr_value_name(cls->repr->val), cls->repr->start, cls->repr->end,
            class_size);

        /* Step inside the class master roster vector to dump all active sub-members */
        for (int i = 0; i < class_size; i++) {
            KlrCongruenceMember *sub_m = vector_get(&cls->members, i);
            ASSERT(sub_m && sub_m->key);

            log_info("[COALESCE]   └─ Member %d: %s, LiveRange: [%d, %d)", i + 1,
                     klr_value_name(sub_m->key->val), sub_m->key->start, sub_m->key->end);
        }
    }

    log_info("[COALESCE] ========================================================");
    log_info("[COALESCE] Snapshot [%s] Summary: Partitioned dataflow tree into %d active classes.",
             phase_tag, class_counter);
    log_info("[COALESCE] ========================================================");
}

/* Collect Final Classes: Traverses the member map and harvests the uniquely converged
 * class containers into the context's classes array. Uses the leader-identity match
 * to ensure absolute uniqueness and zero duplication, preparing for the downstream De-SSA pass.
 */
static void _collect_final_classes(KlrCoalesceContext *ctx)
{
    HashMapIter it = { 0 };

    while (hashmap_next(&ctx->member_map, &it)) {
        KlrCongruenceMember *member = (KlrCongruenceMember *)it.entry;
        if (!member || !member->cls) {
            continue;
        }

        KlrCongruenceClass *cls = member->cls;

        /* only harvest the class block when the traversed
         * member profile matches the supreme Representative (repr) leader itself!
         */
        if (member->key != cls->repr) {
            continue;
        }

        vector_push_back(&ctx->final_classes, &cls);
    }
}

static void klr_coalesce_run_analysis(KlrFunc *fn, KlrCoalesceContext *ctx)
{
    bool changed = true;
    int iteration_count = 0;

    _dump_congruence_classes(ctx, "INITIAL_STATE");

    log_info("[COALESCE] Launching fixed-point iteration pass driver engine...");

    while (changed) {
        changed = false;
        iteration_count++;

        KlrBasicBlock *bb;
        basic_block_foreach(bb, fn) {
            KlrInsn *op;
            insn_foreach(op, bb) {
                if (op->code != OP_IR_PHI) break;

                KlrCongruenceValue *repr_dst = _find_repr(ctx, (KlrValue *)op);

                KlrValue *oper;
                insn_oper_value_foreach(oper, op) {
                    KlrCongruenceValue *repr_src = _find_repr(ctx, oper);
                    /* Try Merge triggers the interval overlap check and migration engine */
                    if (_try_merge(ctx, repr_dst, repr_src)) {
                        changed = true;
                        log_info("[COALESCE] Success: Merged %s with operand %s in iteration %d",
                                 klr_value_name((KlrValue *)op), klr_value_name(oper),
                                 iteration_count);
                    }
                }
            }
        }
    }

    log_info("[COALESCE] Analysis successfully converged after %d total iterations.",
             iteration_count);

    _dump_congruence_classes(ctx, "OPTIMIZED_FINAL_STATE");

    _collect_final_classes(ctx);
}

static int _member_equal_(void *e1, void *e2)
{
    KlrCongruenceMember *m1 = e1;
    KlrCongruenceMember *m2 = e2;
    return m1->key->val == m2->key->val;
}

static void klr_coalesce_context_init(KlrCoalesceContext *ctx)
{
    vector_init_ptr(&ctx->values);
    vector_init_ptr(&ctx->final_classes);
    hashmap_init(&ctx->member_map, _member_equal_);
}

static void klr_coalesce_context_fini(KlrCoalesceContext *ctx)
{
    vector_fini(&ctx->values);
    vector_fini(&ctx->final_classes);
    hashmap_fini(&ctx->member_map, NULL, NULL);
}

/* Re-maps and flattens instruction operands using Def-Use chains.
 * For every non-trivial equivalence class:
 *   1. Creates ONE local in entry block as the class carrier
 *   2. Inserts move %lc, src in predecessors of each eliminated PHI
 *   3. Replaces all uses of eliminated PHIs with %lc
 * Non-PHI members (%a, %1, %3) are NEVER touched.
 * PHI instructions themselves are NOT deleted (De-SSA responsibility).
 */
/*
func @test9(%a int64, %b int64) int64 {
  %bb0(entry):
        %0 = cmpgt %b int64, 0 int64
        branch %0 bool, label %bb1, label %bb2

  %bb1(if-then):                                  ;; preds = %bb0
        %1 = add %a int64, 1 int64
        jmp label %bb2

  %bb2(while-cond):                               ;; preds = %bb1, %bb0
        %x.phi.0 = phi %1 int64, %a int64
        %2 = cmplt %x.phi.0 int64, 10 int64
        branch %2 bool, label %bb3, label %bb4

  %bb3(while-body):                               ;; preds = %bb2, %bb3
        %x.phi.1 = phi %x.phi.0 int64, %3 int64
        %3 = add %x.phi.1 int64, 2 int64
        %4 = cmplt %3 int64, 10 int64
        branch %4 bool, label %bb3, label %bb4

  %bb4(while-end):                                ;; preds = %bb2, %bb3
        %x.phi.2 = phi %x.phi.0 int64, %3 int64
        ret %x.phi.2 int64
}

func @test9(%a int64, %b int64) int64 {
  %bb0(entry):
        %lc = local int64          ; ← Step 1
        move %lc, %a               ; ← %x.phi.0 pred bb0
        %0 = cmpgt %b int64, 0 int64
        branch %0 bool, label %bb1, label %bb2

  %bb1(if-then):
        %1 = add %a int64, 1 int64
        move %lc, %1               ; ← %x.phi.0 pred bb1
        jmp label %bb2

  %bb2(while-cond):
        %x.phi.0 = phi %1 int64, %a int64   ; ← 死 PHI，等待 DCE
        move %lc, %x.phi.0         ; ← %x.phi.1/%x.phi.2 pred bb2
        %2 = cmplt %lc int64, 10 int64      ; ← Use 替换
        branch %2 bool, label %bb3, label %bb4

  %bb3(while-body):
        %x.phi.1 = phi %x.phi.0 int64, %3 int64  ; ← 死 PHI
        %3 = add %lc int64, 2 int64              ; ← Use 替换
        move %lc, %3               ; ← %x.phi.1/%x.phi.2 pred bb3
        %4 = cmplt %3 int64, 10 int64
        branch %4 bool, label %bb3, label %bb4

  %bb4(while-end):
        %x.phi.2 = phi %x.phi.0 int64, %3 int64  ; ← 死 PHI
        ret %lc int64                            ; ← Use 替换
}
*/
static void klr_coalesce_rewrite(KlrFunc *fn, KlrCoalesceContext *ctx)
{
    log_info("[COALESCE] Re-mapping IR operands based on high-efficiency Def-Use chains...");

    KlrCongruenceClass *cls;
    vector_foreach(cls, &ctx->final_classes) {
        ASSERT(cls->repr);

        /* Skip trivial singleton classes — nothing to coalesce */
        if (vector_size(&cls->members) <= 1) continue;

        /* The winning representative value serves as the supreme target across the whole group */
        KlrValue *leader_val = cls->repr->val;

        /* Step 1: Create ONE local per class in entry block.
         * Name derived from leader to ensure uniqueness and debuggability.
         * e.g., leader "%x.phi.0" → local name "lc.x.phi.0"
         */
        char lc_name[64];
        snprintf(lc_name, sizeof(lc_name), "lc.%s", klr_value_name(leader_val) + 1);

        KlrBasicBlock *entry_bb = klr_entry_block(fn);
        KlrBuilder bldr;
        klr_builder_head(&bldr, entry_bb);
        KlrValue *lc = klr_build_local_var(&bldr, leader_val->ts, atom(lc_name));

        log_info("[COALESCE] Created local %s for class led by %s (%d members)",
                 klr_value_name(lc), klr_value_name(leader_val), vector_size(&cls->members));

        /* Step 2: Process each member that is an eliminated PHI */

        Vector done_edges;
        vector_init(&done_edges, sizeof(PhiEdge));

        /*
         * PHASE A: Insert moves (READ-ONLY on PHI operands)
         * All PHI operands are still original SSA Values at this point.
         * No use replacement has occurred yet.
         */
        KlrCongruenceMember *member;
        vector_foreach(member, &cls->members) {
            ASSERT(member->key && member->key->val);

            KlrValue *val = member->key->val;
            if (klr_is_param(val) || klr_is_const(val)) continue;

            ASSERT(klr_is_insn(val));
            KlrInsn *_insn = (KlrInsn *)val;

            /* ONLY process PHI nodes. Non-PHI members are untouched. */
            if (_insn->code != OP_IR_PHI) continue;

            log_info("[COALESCE] Eliminating PHI %s -> replacing uses with %s",
                     klr_value_name(val), klr_value_name(lc));

            /* Insert move in each predecessor for this eliminated PHI */
            for (int i = 0; i < _insn->num_opers; i++) {
                KlrValue *src = insn_oper_value(_insn, i);
                KlrBasicBlock *pred = _insn->phi_preds[i];

                /* Skip self-loop back-edge: phi references itself */
                if (src == val) continue;

                /* Linear de-dup: has this (pred, src) already been
                 * inserted for THIS local? O(done_count) per edge.
                 */
                bool already_done = false;
                PhiEdge *edge;
                vector_foreach_ptr(edge, &done_edges) {
                    if (edge->pred == pred && edge->src == src) {
                        already_done = true;
                        break;
                    }
                }
                if (already_done) continue;

                /* Insert move immediately after the definition of src (if src is
                 * an instruction in pred). This preserves the original instruction
                 * order and minimizes live range overlap for peephole optimization.
                 */

                KlrBuilder bldr;
                if (klr_is_insn(src)) {
                    klr_builder_at(&bldr, (KlrInsn *)src); // insert after instruction
                } else if (pred == entry_bb) {
                    /* pred is entry block: insert after local declarations */
                    klr_builder_at(&bldr, (KlrInsn *)lc);
                } else {
                    klr_builder_head(&bldr, pred); // insert at block start
                }
                klr_build_move(&bldr, lc, src);

                log_info("[COALESCE] Inserted move %s, %s in bb %s", klr_value_name(lc),
                         klr_value_name(src), klr_value_name((KlrValue *)pred));

                /* Record this edge as processed */
                PhiEdge new_edge = { .pred = pred, .src = src };
                vector_push_back(&done_edges, &new_edge);
            }
        }

        /*
         * PHASE B: Replace uses (MUTATION)
         * Moves are already inserted with correct original sources.
         * Now safe to mutate Use chains without affecting Phase A.
         */
        vector_foreach(member, &cls->members) {
            ASSERT(member->key && member->key->val);
            KlrValue *val = member->key->val;

            if (klr_is_const(val)) continue;

            if (klr_is_insn(val)) {
                KlrInsn *_insn = (KlrInsn *)val;

                /* ONLY process PHI nodes. Non-PHI members are untouched. */
                if (_insn->code == OP_IR_PHI) {
                    /* Replace ONLY uses of this eliminated PHI with %lc */
                    replace_all_uses_with(lc, val);
                    continue;
                }
                // Non-PHI instructions or parameters, fall through to the below logic.
            }

            /* Replace non-phi uses of this value with %lc.
             * Skip move(lc, val)'s src to avoid move(lc, lc).
             *
             * example:
             *   %1 = xxxx
             *   move %lc, %1        ← %1 is src, %lc is dst
             * move's %1 cannot be replaced by %lc, otherwise it becomes move(lc, lc).
             */
            KlrUse *use, *next;
            use_foreach_safe(use, next, val) {
                KlrInsn *insn = use->insn;

                /* Skip PHI operands — they are edge bindings, not replaceable uses */
                if (insn->code == OP_IR_PHI) continue;

                /* Preserve move(lc, val)'s src to avoid move(lc, lc) */
                if (insn->code == OP_MOVE) {
                    KlrValue *dst = insn_oper_value(insn, 0);
                    KlrValue *src = insn_oper_value(insn, 1);
                    if (src == val && dst == lc) continue;
                }
                set_operand(use->oper, use->insn, lc);
            }
        }

        vector_fini(&done_edges);
    }

    log_info("[COALESCE] Core IR operand mutation complete. All redundant definitions flattened.");
}

/* phi coalescing optimization + de-ssa pass */
int klr_phi_coalescing_exit_ssa_pass(KlrFunc *fn)
{
    log_info("Starting phi coalescing pass for function: %s", fn->name);

    KlrCoalesceContext ctx;
    klr_coalesce_context_init(&ctx);

    _assign_coord(fn);

    // 1. Collect all unique values (PHIs and their operands) and build intervals [start, end)
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *op;
        insn_foreach(op, bb) {
            if (op->code != OP_IR_PHI) break;

            _register_value((KlrValue *)op, &ctx);

            KlrValue *operand;
            insn_oper_value_foreach(operand, op) {
                _register_value(operand, &ctx);
            }
        }
    }

    // 2. Run transactional fixpoint convergence engine
    klr_coalesce_run_analysis(fn, &ctx);

    // 3. Rewrite the IR based on the analysis
    klr_coalesce_rewrite(fn, &ctx);

    // 4. Reclaim context resources
    klr_coalesce_context_fini(&ctx);

    log_info("Finished phi coalescing pass for function: %s", fn->name);

    return 0;
}

#ifdef __cplusplus
}
#endif
