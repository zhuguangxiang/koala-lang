/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Fully unroll the small counted loops, e.g.
 *
 *   for i in range(4) { s += i }          var i = 0
 *                                         while i < 3 { s += i; i += 1 }
 *
 * It runs on the optimized (non-SSA) IR and needs the dominator tree. A loop is the natural
 * loop of a header H: the latches are the preds of H dominated by H, the body is H plus the
 * blocks reaching a latch without passing H. A loop is unrolled if:
 *   - it is an innermost loop (no other back edge in its body),
 *   - H has only one pred P outside of the loop,
 *   - no value defined in the loop is used outside of it,
 *   - it has an exit test 'branch (cmp %i, C)' in a block E dominating all latches, %i is a
 *     signed int local, defined only once in the loop by 'move %i, (add|sub %i, C)' in a
 *     block M dominating all latches, and the initial value of %i is a constant 'move %i, C'
 *     before the loop,
 *   - the exit test is taken at the K-th entry of H, K <= UNROLL_MAX_TRIPS, and the unrolled
 *     body has no more than UNROLL_MAX_INSNS instructions.
 *
 * The loop is peeled K times: P jumps to the first copy, the back edges of the k-th copy jump
 * to the (k+1)-th copy, and the ones of the last copy jump to the original loop. The exits of
 * every copy are kept, so the result is always correct. The reads of %i in a copy are replaced
 * by its constant value in that trip, as the opt only propagates a 'var' in one block. The
 * following opt folds the exit test of the last copy and removes the now unreachable loop.
 *
 * An inner loop is unrolled first, the outer loop becomes innermost after the opt and is
 * unrolled in the next round.
 */

#include "cmd.h"
#include "domtree.h"
#include "mm.h"
#include "opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UNROLL_MAX_TRIPS     16
#define UNROLL_MAX_INSNS     256
#define UNROLL_MAX_INIT_WALK 8
#define UNROLL_MAX_PER_FUNC  8

typedef struct _PtrEntry {
    HashMapEntry hnode;
    void *key;
    void *val;
} PtrEntry;

/* the defs of a value are not in its use list */
#define def_foreach(use, val) list_foreach(use, use_link, &(val)->def_list)

static int _ptr_equal_(void *a, void *b) { return ((PtrEntry *)a)->key == ((PtrEntry *)b)->key; }

static void ptr_map_put(HashMap *map, void *key, void *val)
{
    PtrEntry *entry = mm_alloc_obj(entry);
    hashmap_entry_init(entry, mem_hash(&key, sizeof(void *)));
    entry->key = key;
    entry->val = val;
    PtrEntry *old = hashmap_put(map, entry);
    if (old) mm_free(old);
}

static void *ptr_map_get(HashMap *map, void *key)
{
    PtrEntry k = { .key = key };
    hashmap_entry_init(&k, mem_hash(&key, sizeof(void *)));
    PtrEntry *entry = hashmap_get(map, &k);
    return entry ? entry->val : NULL;
}

static void _free_entry_(void *e, void *arg) { mm_free(e); }

static void ptr_map_fini(HashMap *map) { hashmap_fini(map, _free_entry_, NULL); }

typedef struct _Loop {
    KlrDomTree *kdt;
    KlrBasicBlock *header;
    /* the only pred of the header outside of the loop */
    KlrBasicBlock *preheader;
    /* dom index -> in the loop */
    char *in_loop;
    /* the blocks of the loop, in the function order */
    KlrBasicBlock **blocks;
    int num_blocks;
    int num_insns;
    /* trip count, the number of the header entries */
    int trips;
    /* the counter, its only def in the loop, and its value on the k-th entry of the header */
    KlrValue *var;
    KlrInsn *step_move;
    int num_vals;
    int64_t vals[UNROLL_MAX_TRIPS + 1];
} Loop;

static int in_loop(Loop *loop, KlrBasicBlock *bb) { return loop->in_loop[bb->dom_index]; }

static void loop_fini(Loop *loop)
{
    mm_free(loop->in_loop);
    mm_free(loop->blocks);
}

/* the natural loop of `header`, returns 0 if `header` is not a loop header */
static int find_loop(Loop *loop, KlrDomTree *kdt, KlrBasicBlock *header)
{
    memset(loop, 0, sizeof(*loop));
    loop->kdt = kdt;
    loop->header = header;
    loop->in_loop = mm_alloc(kdt->num_blocks);

    KlrBasicBlock **stack = mm_alloc(sizeof(KlrBasicBlock *) * kdt->num_blocks);
    int top = 0;

    loop->in_loop[header->dom_index] = 1;
    KlrEdge *edge;
    edge_in_foreach(edge, header) {
        KlrBasicBlock *src = edge->src;
        if (klr_reachable(kdt, src) && klr_dominates(kdt, header, src)) stack[top++] = src;
    }

    int found = top > 0;
    while (top > 0) {
        KlrBasicBlock *bb = stack[--top];
        if (loop->in_loop[bb->dom_index]) continue;
        loop->in_loop[bb->dom_index] = 1;
        edge_in_foreach(edge, bb) {
            KlrBasicBlock *src = edge->src;
            if (klr_reachable(kdt, src) && !loop->in_loop[src->dom_index]) stack[top++] = src;
        }
    }
    mm_free(stack);

    if (!found) return 0;

    loop->blocks = mm_alloc(sizeof(KlrBasicBlock *) * kdt->num_blocks);
    KlrBasicBlock *bb;
    basic_block_foreach(bb, kdt->fn) {
        if (in_loop(loop, bb)) loop->blocks[loop->num_blocks++] = bb;
    }
    return 1;
}

/* the shape of the loop, not the trip count */
static int check_loop(Loop *loop)
{
    KlrDomTree *kdt = loop->kdt;
    KlrBasicBlock *header = loop->header;

    KlrEdge *edge;
    edge_in_foreach(edge, header) {
        KlrBasicBlock *src = edge->src;
        if (!klr_reachable(kdt, src) || in_loop(loop, src)) continue;
        if (loop->preheader) return 0;
        loop->preheader = src;
    }
    // the entry block is a new block, not the first copy of the loop
    if (!loop->preheader || loop->preheader == kdt->fn->sbb) return 0;

    for (int i = 0; i < loop->num_blocks; i++) {
        KlrBasicBlock *bb = loop->blocks[i];
        edge_out_foreach(edge, bb) {
            KlrBasicBlock *dst = edge->dst;
            // an inner loop
            if (dst != header && in_loop(loop, dst) && klr_dominates(kdt, dst, bb)) return 0;
        }

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn_is(insn, OP_IR_PHI)) return 0;
            loop->num_insns++;
            KlrUse *use;
            use_foreach(use, (KlrValue *)insn) {
                if (!in_loop(loop, use->insn->bb)) return 0;
            }
            def_foreach(use, (KlrValue *)insn) {
                if (!in_loop(loop, use->insn->bb)) return 0;
            }
        }
    }
    return 1;
}

static int dominates_latches(Loop *loop, KlrBasicBlock *bb)
{
    KlrEdge *edge;
    edge_in_foreach(edge, loop->header) {
        KlrBasicBlock *src = edge->src;
        if (!klr_reachable(loop->kdt, src) || !in_loop(loop, src)) continue;
        if (!klr_dominates(loop->kdt, bb, src)) return 0;
    }
    return 1;
}

/* the value of an int constant, sign-extended from the width of its type */
static int const_int_value(KlrValue *val, int64_t *out)
{
    if (!klr_is_const(val)) return 0;
    KlrConst *kc = (KlrConst *)val;
    if (kc->which != CONST_INT) return 0;
    *out = (int64_t)kc->ival;
    return 1;
}

static int insn_index(KlrInsn *insn)
{
    int idx = 0;
    KlrInsn *it;
    insn_foreach(it, insn->bb) {
        if (it == insn) return idx;
        idx++;
    }
    UNREACHABLE();
    return -1;
}

typedef struct _CountedLoop {
    KlrValue *var;
    int64_t min, max;
    /* exit test: var <rel> limit */
    int rel;
    int64_t limit;
    /* exit if the test is true */
    int exit_on_true;
    /* the test reads var before the step */
    int test_before_step;
    /* step: var = var + step */
    int64_t step;
    int64_t init;
} CountedLoop;

static int swap_rel(int rel)
{
    switch (rel) {
        case OP_BINARY_LT:
            return OP_BINARY_GT;
        case OP_BINARY_LE:
            return OP_BINARY_GE;
        case OP_BINARY_GT:
            return OP_BINARY_LT;
        case OP_BINARY_GE:
            return OP_BINARY_LE;
        default:
            return rel;
    }
}

static int is_rel(int code)
{
    return code == OP_BINARY_LT || code == OP_BINARY_LE || code == OP_BINARY_GT ||
           code == OP_BINARY_GE || code == OP_BINARY_EQ || code == OP_BINARY_NE;
}

static int eval_rel(int rel, int64_t a, int64_t b)
{
    switch (rel) {
        case OP_BINARY_LT:
            return a < b;
        case OP_BINARY_LE:
            return a <= b;
        case OP_BINARY_GT:
            return a > b;
        case OP_BINARY_GE:
            return a >= b;
        case OP_BINARY_EQ:
            return a == b;
        default:
            return a != b;
    }
}

/* the exit test 'branch (cmp %i, C)' in `bb` */
static int match_exit_test(Loop *loop, KlrBasicBlock *bb, CountedLoop *cl, KlrInsn **cmp_out)
{
    if (!dominates_latches(loop, bb)) return 0;

    KlrInsn *br = insn_last(bb);
    if (!br || !insn_is(br, OP_IR_JMP_COND)) return 0;
    KlrBasicBlock *then_bb = (KlrBasicBlock *)insn_oper_value(br, 2);
    KlrBasicBlock *else_bb = (KlrBasicBlock *)insn_oper_value(br, 3);
    int then_in = in_loop(loop, then_bb);
    int else_in = in_loop(loop, else_bb);
    if (then_in == else_in) return 0;
    cl->exit_on_true = !then_in;

    KlrValue *cond = insn_oper_value(br, 0);
    if (!klr_is_insn(cond)) return 0;
    KlrInsn *cmp = (KlrInsn *)cond;
    if (cmp->bb != bb || !is_rel(cmp->code) || cmp->num_opers != 2) return 0;

    KlrValue *lhs = insn_oper_value(cmp, 0);
    KlrValue *rhs = insn_oper_value(cmp, 1);
    cl->rel = cmp->code;
    if (klr_is_local(lhs) && const_int_value(rhs, &cl->limit)) {
        cl->var = lhs;
    } else if (klr_is_local(rhs) && const_int_value(lhs, &cl->limit)) {
        cl->var = rhs;
        cl->rel = swap_rel(cl->rel);
    } else {
        return 0;
    }

    TypeSpec *ts = cl->var->ts;
    if (!ts || !type_is_int(ts)) return 0;
    int bits = int_type_width(ts) * 8;
    if (bits <= 0 || bits > 64) return 0;
    cl->max = (bits == 64) ? INT64_MAX : (((int64_t)1 << (bits - 1)) - 1);
    cl->min = -cl->max - 1;

    *cmp_out = cmp;
    return 1;
}

/* the only def of the var in the loop 'move %i, (add|sub %i, C)', returns the move */
static KlrInsn *match_step(Loop *loop, CountedLoop *cl)
{
    KlrInsn *move = NULL;
    KlrUse *use;
    def_foreach(use, cl->var) {
        if (!in_loop(loop, use->insn->bb)) continue;
        if (move) return NULL;
        move = use->insn;
    }
    if (!move || !insn_is(move, OP_MOVE) || insn_oper_value(move, 0) != cl->var) return NULL;
    if (!dominates_latches(loop, move->bb)) return NULL;

    KlrValue *src = insn_oper_value(move, 1);
    if (!klr_is_insn(src) || klr_is_local(src)) return NULL;
    KlrInsn *op = (KlrInsn *)src;
    if (op->bb != move->bb || op->num_opers != 2) return NULL;
    if (!insn_is(op, OP_BINARY_ADD) && !insn_is(op, OP_BINARY_SUB)) return NULL;
    if (insn_index(op) > insn_index(move)) return NULL;

    KlrValue *lhs = insn_oper_value(op, 0);
    KlrValue *rhs = insn_oper_value(op, 1);
    int64_t c;
    if (lhs == cl->var && const_int_value(rhs, &c)) {
        if (insn_is(op, OP_BINARY_SUB)) {
            if (c == INT64_MIN) return NULL;
            c = -c;
        }
    } else if (rhs == cl->var && insn_is(op, OP_BINARY_ADD) && const_int_value(lhs, &c)) {
        // 'add C, %i'
    } else {
        return NULL;
    }
    cl->step = c;
    return move;
}

/* the constant value of the var on entry of the loop: 'move %i, C' before the loop */
static int match_init(Loop *loop, CountedLoop *cl)
{
    KlrBasicBlock *bb = loop->preheader;
    for (int n = 0; n < UNROLL_MAX_INIT_WALK; n++) {
        KlrInsn *insn;
        insn_foreach_reverse(insn, bb) {
            for (int i = 0; i < insn->num_opers; i++) {
                KlrOper *oper = insn_operand(insn, i);
                if (!oper->use.is_def || oper->use.ref != cl->var) continue;
                if (!insn_is(insn, OP_MOVE) || i != 0) return 0;
                return const_int_value(insn_oper_value(insn, 1), &cl->init);
            }
        }
        if (bb->num_inedges != 1) return 0;
        bb = edge_in_first(bb)->src;
        if (bb == loop->kdt->fn->sbb) return 0;
    }
    return 0;
}

static int step_value(CountedLoop *cl, int64_t v, int64_t *out)
{
    if (__builtin_add_overflow(v, cl->step, out)) return 0;
    return *out >= cl->min && *out <= cl->max;
}

/*
 * The number of the header entries until the exit test is taken, 0 if too many. The value of
 * the counter on the k-th entry is saved in vals[k - 1], and the one after the last step is
 * saved if it does not overflow.
 */
static int simulate(Loop *loop, CountedLoop *cl)
{
    int64_t v = cl->init;
    if (v < cl->min || v > cl->max) return 0;
    for (int trip = 1; trip <= UNROLL_MAX_TRIPS; trip++) {
        loop->vals[trip - 1] = v;
        if (cl->test_before_step && eval_rel(cl->rel, v, cl->limit) == cl->exit_on_true) {
            // the step of the last trip is not executed
            loop->num_vals = trip;
            if (step_value(cl, v, &loop->vals[trip])) loop->num_vals++;
            return trip;
        }
        if (!step_value(cl, v, &v)) return 0;
        if (!cl->test_before_step && eval_rel(cl->rel, v, cl->limit) == cl->exit_on_true) {
            loop->vals[trip] = v;
            loop->num_vals = trip + 1;
            return trip;
        }
    }
    return 0;
}

static int trip_count(Loop *loop)
{
    for (int i = 0; i < loop->num_blocks; i++) {
        KlrBasicBlock *exit_bb = loop->blocks[i];
        CountedLoop cl;
        memset(&cl, 0, sizeof(cl));
        KlrInsn *cmp;
        if (!match_exit_test(loop, exit_bb, &cl, &cmp)) continue;
        KlrInsn *move = match_step(loop, &cl);
        if (!move) continue;
        if (exit_bb == move->bb) {
            cl.test_before_step = insn_index(cmp) < insn_index(move);
        } else {
            cl.test_before_step = klr_dominates(loop->kdt, exit_bb, move->bb);
        }
        if (!match_init(loop, &cl)) continue;
        int trips = simulate(loop, &cl);
        if (trips > 0) {
            loop->var = cl.var;
            loop->step_move = move;
            return trips;
        }
    }
    return 0;
}

/* the target of the block `bb` in the k-th copy */
static KlrBasicBlock *map_block(Loop *loop, HashMap *maps, KlrBasicBlock **headers, int k,
                                KlrBasicBlock *bb)
{
    // a back edge goes to the next copy, the last one goes to the original loop
    if (bb == loop->header) return (k + 1 < loop->trips) ? headers[k + 1] : loop->header;
    KlrBasicBlock *mapped = ptr_map_get(&maps[k], bb);
    return mapped ? mapped : bb;
}

/*
 * The constant value of the counter read by `insn` in the k-th copy, NULL if unknown. In a trip,
 * a block before the step reaches the step, a block after it is dominated by the step.
 */
static KlrValue *counter_value(Loop *loop, int k, KlrInsn *insn)
{
    if (insn_or(insn, OP_GLOBAL_SET, OP_GLOBAL_GET)) return NULL;
    if (insn_is(insn, OP_IR_CALL) && (insn->flags & KLR_INSN_FLAGS_CONST)) return NULL;

    KlrInsn *move = loop->step_move;
    int after;
    if (insn->bb == move->bb) {
        after = insn_index(insn) > insn_index(move);
    } else {
        after = klr_dominates(loop->kdt, move->bb, insn->bb);
    }

    int idx = k + after;
    if (idx >= loop->num_vals) return NULL;
    return klr_const_int((uint64_t)loop->vals[idx], loop->var->ts, loop->kdt->fn->module);
}

static void peel(Loop *loop)
{
    int trips = loop->trips;
    HashMap *maps = mm_alloc(sizeof(HashMap) * trips);
    KlrBasicBlock **headers = mm_alloc(sizeof(KlrBasicBlock *) * trips);

    // create the blocks of all copies, before the header in the function order
    for (int k = 0; k < trips; k++) {
        hashmap_init(&maps[k], _ptr_equal_);
        for (int i = 0; i < loop->num_blocks; i++) {
            KlrBasicBlock *bb = loop->blocks[i];
            KlrBasicBlock *copy = klr_add_block_before(loop->header, bb->name);
            ptr_map_put(&maps[k], bb, copy);
            if (bb == loop->header) headers[k] = copy;
        }
    }

    for (int k = 0; k < trips; k++) {
        // clone all instructions before setting the operands, so an operand is always mapped
        for (int i = 0; i < loop->num_blocks; i++) {
            KlrBasicBlock *bb = loop->blocks[i];
            KlrBuilder bldr;
            klr_builder_end(&bldr, (KlrBasicBlock *)ptr_map_get(&maps[k], bb));
            KlrInsn *insn;
            insn_foreach(insn, bb) {
                KlrInsn *clone = klr_build_clone(&bldr, insn, insn->ts);
                // the constants of the counter are variables for the language
                clone->flags |= KLR_INSN_FLAGS_INLINED;
                ptr_map_put(&maps[k], insn, clone);
            }
        }

        for (int i = 0; i < loop->num_blocks; i++) {
            KlrBasicBlock *bb = loop->blocks[i];
            KlrInsn *insn;
            insn_foreach(insn, bb) {
                KlrInsn *clone = ptr_map_get(&maps[k], insn);
                for (int j = 0; j < insn->num_opers; j++) {
                    KlrValue *val = insn_oper_value(insn, j);
                    KlrValue *mapped = val;
                    KlrValue *c;
                    if (val && val == loop->var && !insn_operand(insn, j)->use.is_def &&
                        (c = counter_value(loop, k, insn))) {
                        mapped = c;
                    } else if (val && klr_is_block(val)) {
                        mapped = (KlrValue *)map_block(loop, maps, headers, k,
                                                       (KlrBasicBlock *)val);
                    } else if (val) {
                        KlrValue *v = ptr_map_get(&maps[k], val);
                        if (v) mapped = v;
                    }
                    klr_init_clone_oper(clone, insn, j, mapped);
                }
            }

            KlrBasicBlock *copy = ptr_map_get(&maps[k], bb);
            KlrEdge *edge;
            edge_out_foreach(edge, bb) {
                klr_link_edge(copy, map_block(loop, maps, headers, k, edge->dst));
            }
        }
    }

    // the preheader enters the first copy
    KlrBasicBlock *pre = loop->preheader;
    KlrInsn *term = insn_last(pre);
    if (term) {
        for (int j = 0; j < term->num_opers; j++) {
            if (insn_oper_value(term, j) == (KlrValue *)loop->header) {
                set_operand_at(term, j, (KlrValue *)headers[0]);
            }
        }
    }

    int num_succs = pre->num_outedges;
    KlrBasicBlock **succs = mm_alloc(sizeof(KlrBasicBlock *) * (num_succs + 1));
    int n = 0;
    KlrEdge *edge;
    edge_out_foreach(edge, pre) {
        succs[n++] = (edge->dst == loop->header) ? headers[0] : edge->dst;
    }
    klr_remove_all_out_edges(pre);
    for (int i = 0; i < n; i++) klr_link_edge(pre, succs[i]);
    mm_free(succs);

    for (int k = 0; k < trips; k++) ptr_map_fini(&maps[k]);
    mm_free(maps);
    mm_free(headers);
}

/* unroll one loop of `fn`, skips the headers in `done` */
static int unroll_one(KlrFunc *fn, KlrBasicBlock **done, int num_done)
{
    klr_func_build_domtree(fn);
    KlrDomTree *kdt = fn->domtree;

    int changed = 0;
    for (int i = 0; i < kdt->num_blocks && !changed; i++) {
        KlrBasicBlock *header = kdt->blocks[i];
        if (header == fn->sbb || header == fn->ebb || !klr_reachable(kdt, header)) continue;

        int skip = 0;
        for (int j = 0; j < num_done; j++) {
            if (done[j] == header) skip = 1;
        }
        if (skip) continue;

        Loop loop;
        if (!find_loop(&loop, kdt, header)) {
            loop_fini(&loop);
            continue;
        }

        if (check_loop(&loop)) {
            loop.trips = trip_count(&loop);
            if (loop.trips > 0 && loop.trips * loop.num_insns <= UNROLL_MAX_INSNS) {
                peel(&loop);
                done[num_done] = header;
                changed = 1;
            }
        }
        loop_fini(&loop);
    }

    // the CFG is changed, the tree is stale
    klr_func_free_domtree(fn);
    return changed;
}

static int unroll_func(KlrFunc *fn)
{
    KlrBasicBlock *done[UNROLL_MAX_PER_FUNC];
    int count = 0;
    while (count < UNROLL_MAX_PER_FUNC && unroll_one(fn, done, count)) count++;

    if (count > 0 && dump_unroll_ir_enabled()) {
        fprintf(stdout, "--- IR Dump After unroll ---\n");
        klr_print_func(fn, stdout);
    }
    return count;
}

int kl_unroll(KlrModule *m)
{
    if (!m || m->errors > 0) return 0;

    int count = 0;
    KlrFunc *fn;
    func_foreach(fn, m) {
        if (fn) count += unroll_func(fn);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        func_foreach(fn, kls) {
            count += unroll_func(fn);
        }
    }
    return count;
}

#ifdef __cplusplus
}
#endif
