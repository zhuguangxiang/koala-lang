/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * lvn.c -- Local Value Numbering (LVN), built on the SSA IR.
 *
 * Approach: process one basic block at a time (block order is irrelevant: there
 * is no cross-block state at all). Within a block we keep an expression table
 * (HashMap) keyed by (opcode, operand1, operand2), whose value is the first
 * defining instruction of that expression (the leader). Any later instruction
 * that hits a key is redundant; for it we only do
 *     replace_all_uses_with(leader, dup)
 * Structural removal (the dup itself and its cascading dead code) is left
 * entirely to the dce pass and is not done here.
 *
 * Why RAUW can be safe function-wide:
 *     leader and dup are in the same basic block and leader comes first. Under
 *     SSA every use of dup is dominated by dup's definition, and leader is
 *     necessarily executed before the block exit, so leader dominates every use
 *     of dup (including phi operands and branch conditions in successor blocks).
 *
 * We reuse the existing facilities directly and invent no new data structures:
 *     - A KlrInsn IS the value it defines (KLR_VALUE_HEAD), so def-use chains
 *       come for free: use replace_all_uses_with / klr_erase_insn, and operand
 *       rewriting is handled by use_list;
 *     - Constants are interned per module (KlrConst pointers are unique), so
 *       pointer equality means value equality; no separate constant value-
 *       numbering table is needed;
 *     - No "vn table" is needed either: once dup is RAUW'd it is deleted by dce,
 *       after which any key can only contain the leader's own pointer; the
 *       equivalence is maintained by use_list (when no phi is involved,
 *       congruence propagation = RAUW, and the value pointer is its own number).
 *
 * v1 scope (explicitly NOT done):
 *     - phi is not handled (skipped by request; as far as the table is concerned
 *       a phi result is just an ordinary operand pointer);
 *     - pure value ops OP_BINARY_* / OP_UNARY_* go into the exprs table;
 *       field reads OP_GET_FIELD(_EXT) go into the loads table (see "memory-
 *       dependent reads" below);
 *       OP_MOVE's dst is a local slot (a storage definition, not an SSA copy),
 *       so it is excluded; but an operand that is a local reads the slot's
 *       current value, so a MOVE into a local retires every entry (in both
 *       tables) that reads it, e.g. `i + 1` before and after `move %i, ...`;
 *       OP_IR_LOCAL/PHI/CALL/CAST/SELECT, the loadk family and terminators are
 *       all opaque;
 *     - identity rules and constant folding are left to existing passes; this
 *       pass only does congruence elimination;
 *     - cross-block congruence (which needs dominance checks) belongs to the
 *       next step, GVN, and is out of scope for this pass.
 *
 * Ordering requirement: this pass MUST run before cfg_bb_opt (klr_merge_block_pass).
 * After RAUW the redundant comparison instructions become "syntactically
 * identical", so tail-merge can match them without any change.
 *
 * The UNWRAPPED bit is NOT part of the key: it is a static-type-view annotation
 * (a T? known to be non-null, treated as T) and does not change which operand
 * value is read; for pure value ops the result is unrelated to that bit.
 * Moreover the low-level RAUW rewriting automatically sets/clears the flag on
 * the users (see klr/insn.c); putting it in the key would make a pair of
 * just-merged twin instructions miss each other because of differing
 * annotations, which only hurts fidelity.
 *
 * Invalidation rule for memory-dependent reads (get_field):
 *     Two get_field(p, x) are congruent only if nobody rewrites p's x field in
 *     between. A write can happen through an aliased object (q.x = v, where q
 *     and p are the same instance) or even through a callee (which can modify
 *     reachable objects); aliasing is invisible in the key, so we adopt a
 *     conservative global invalidation: on any heap write
 *     (SET_FIELD(_EXT)/SEQ_SET(_IMM)/MAP_SET) or any call (OP_IR_CALL, listed
 *     together with OP_PRINT, since printing an object may trigger a formatting
 *     path), the entire loads table is cleared. The exprs table is unaffected --
 *     pure value ops do not depend on memory. When the obj operand is a local,
 *     a MOVE into that local retires the entry like any other read of the local.
 */

#include "log.h"
#include "mm.h"
#include "opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Expression table node: key (code, op1, op2) -> leader instruction */
typedef struct {
    HashMapEntry entry;
    KlrInsn *leader; /* NULL: retired, an operand local was written after it */
    OpCode code;
    KlrValue *op1;
    KlrValue *op2; /* NULL for unary instructions */
} LvnExpr;

typedef struct {
    HashMap exprs; /* pure value-op table (per block; init/fini each block) */
    HashMap loads; /* memory-dependent read table (per block; fully cleared on heap write/call) */
    int redundant; /* number of redundant insns hit (count only; removal is done by dce) */
} LVNContext;

static uint64_t lvn_expr_hash(OpCode code, KlrValue *op1, KlrValue *op2)
{
    struct {
        uintptr_t code, a, b;
    } buf = { (uintptr_t)code, (uintptr_t)op1, (uintptr_t)op2 };
    return (uint64_t)mem_hash(&buf, sizeof(buf));
}

/* e1: the node in the table; e2: the probe key (same LvnExpr layout) */
static int lvn_expr_equal(void *_e1, void *_e2)
{
    LvnExpr *e1 = _e1;
    LvnExpr *e2 = _e2;
    return e1->code == e2->code && e1->op1 == e2->op1 && e1->op2 == e2->op2;
}

static void lvn_expr_free(void *e, void *arg)
{
    UNUSED(arg);
    mm_free((LvnExpr *)e);
}

/* Numberable pure value ops: binary */
static int lvn_is_numberable(OpCode code)
{
    switch (code) {
        case OP_BINARY_ADD:
        case OP_BINARY_SUB:
        case OP_BINARY_MUL:
        case OP_BINARY_DIV:
        case OP_BINARY_MOD:
        case OP_BINARY_AND:
        case OP_BINARY_OR:
        case OP_BINARY_XOR:
        case OP_BINARY_SHL:
        case OP_BINARY_SHR:
        case OP_BINARY_EQ:
        case OP_BINARY_NE:
        case OP_BINARY_LT:
        case OP_BINARY_LE:
        case OP_BINARY_GT:
        case OP_BINARY_GE:
        case OP_UNARY_PLUS:
        case OP_UNARY_NEG:
        case OP_UNARY_NOT:
            return 1;
        default:
            return 0;
    }
}

/* Commutative ops (used to canonicalize key order); lt/le/gt/ge/shl/shr/sub/div/mod are not
 * equivalent */
static int lvn_is_commutative(OpCode code)
{
    switch (code) {
        case OP_BINARY_ADD:
        case OP_BINARY_MUL:
        case OP_BINARY_AND:
        case OP_BINARY_OR:
        case OP_BINARY_XOR:
        case OP_BINARY_EQ:
        case OP_BINARY_NE:
            return 1;
        default:
            return 0;
    }
}

/* Memory-dependent pure reads: numberable, but subject to the invalidation rule */
static int lvn_is_load_numberable(OpCode code)
{
    switch (code) {
        case OP_GET_FIELD:
        case OP_GET_FIELD_EXT:
            return 1;
        default:
            return 0;
    }
}

/* Instructions that may change the contents of any heap object: clear the loads table on reaching
 * one. Pure value ops / local slot writes (MOVE) are not included; OP_NEW only constructs a new
 * object, so it is not listed either. */
static int lvn_invalidates_loads(KlrInsn *insn)
{
    switch (insn->code) {
        case OP_SET_FIELD:
        case OP_SET_FIELD_EXT:
        case OP_SEQ_SET:
        case OP_SEQ_SET_IMM:
        case OP_SEQ_SET_SLICE:
        case OP_IR_CALL:
        case OP_FMT:
            return 1;
        default:
            return 0;
    }
}

/* Single instruction: RAUW on hit (return 1), otherwise register as leader; map is exprs or loads
 */
static int lvn_number_insn(LVNContext *ctx, KlrInsn *insn, HashMap *map)
{
    ASSERT(insn->num_opers >= 1 && insn->num_opers <= 2);

    KlrValue *op1 = insn_oper_value(insn, 0);
    KlrValue *op2 = insn->num_opers == 2 ? insn_oper_value(insn, 1) : NULL;

    /* Canonicalize key order for commutative ops; only reorder the key, not the instruction itself.
     * Pointer order is a deterministic total order, independent of the actual memory addresses. */
    if (op2 && lvn_is_commutative(insn->code) && (uintptr_t)op1 > (uintptr_t)op2) {
        KlrValue *tmp = op1;
        op1 = op2;
        op2 = tmp;
    }

    uint64_t hash = lvn_expr_hash(insn->code, op1, op2);

    LvnExpr probe;
    hashmap_entry_init(&probe.entry, hash);
    probe.leader = NULL;
    probe.code = insn->code;
    probe.op1 = op1;
    probe.op2 = op2;

    LvnExpr *hit = (LvnExpr *)hashmap_get(map, &probe);
    if (hit && !hit->leader) {
        /* the old leader read a local that was written since: this insn is the new leader */
        hit->leader = insn;
        return 0;
    }
    if (hit) {
        /* Redundant: leader is earlier in the same block => RAUW is safe function-wide (see the
         * file-header argument). The same holds when a div/mod divisor is the constant 0: if leader
         * traps we never reach dup; if we do reach it, leader already succeeded, so dup introduces
         * no new trap. */
        KlrValue *leader = (KlrValue *)hit->leader;
        ASSERT(leader != (KlrValue *)insn);
        log_info("lvn: remove redundant insn, leader used:");
        replace_all_uses_with(leader, (KlrValue *)insn);
        ctx->redundant++;
        return 1;
    }

    LvnExpr *e = mm_alloc(sizeof(LvnExpr));
    hashmap_entry_init(&e->entry, hash);
    e->leader = insn;
    e->code = insn->code;
    e->op1 = op1;
    e->op2 = op2;
    hashmap_put_only(map, e); /* just probed and missed, so no duplicate */
    return 0;
}

static void lvn_clear_loads(LVNContext *ctx)
{
    hashmap_fini(&ctx->loads, lvn_expr_free, NULL);
    hashmap_init(&ctx->loads, lvn_expr_equal);
}

/* A MOVE writes a local slot: an expression that reads the local as an operand
   computes a different value after it, so its entry can no longer be a leader. */
static void lvn_retire_local(HashMap *map, KlrValue *local)
{
    HashMapIter it;
    hashmap_iter_init(map, &it, 0, 1);
    while (hashmap_iter_next(&it)) {
        LvnExpr *e = (LvnExpr *)hashmap_iter_entry(&it);
        if (e->op1 == local || e->op2 == local) e->leader = NULL;
    }
}

static void lvn_bb(LVNContext *ctx, KlrBasicBlock *bb)
{
    hashmap_init(&ctx->exprs, lvn_expr_equal);
    hashmap_init(&ctx->loads, lvn_expr_equal);

    KlrInsn *insn, *nxt;
    insn_foreach_safe(insn, nxt, bb) {
        if (insn_is_terminator(insn)) continue;
        if (lvn_invalidates_loads(insn)) {
            lvn_clear_loads(ctx);
            continue;
        }
        if (insn->code == OP_MOVE) {
            KlrValue *dst = insn_oper_value(insn, 0);
            lvn_retire_local(&ctx->exprs, dst);
            lvn_retire_local(&ctx->loads, dst);
            continue;
        }
        if (lvn_is_numberable(insn->code)) {
            lvn_number_insn(ctx, insn, &ctx->exprs);
        } else if (lvn_is_load_numberable(insn->code)) {
            lvn_number_insn(ctx, insn, &ctx->loads);
        }
    }

    hashmap_fini(&ctx->exprs, lvn_expr_free, NULL);
    hashmap_fini(&ctx->loads, lvn_expr_free, NULL);
}

/*
 * LVN pass: do local value numbering block by block. Returns 1 if redundancy was
 * found (a subsequent dce needs to do the cutting).
 *
 * Block order is irrelevant: the expression tables are built/destroyed per block,
 * congruence is propagated only by within-block RAUW, and there is no cross-block
 * state, so we can just traverse fn->bb_list in its original order.
 *
 * This pass only rewrites references (RAUW); it does not delete instructions. The
 * dup and its cascading dead code are all cleared by dce.
 */
int klr_lvn_pass(KlrFunc *fn, void *data)
{
    UNUSED(data);
    log_info("[lvn] on func '%%%s'", fn->name);

    LVNContext ctx = {
        .redundant = 0,
    };

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) lvn_bb(&ctx, bb);

    log_info("[lvn] '%s': %d redundant insn(s) found (to be erased by dce)", fn->name,
             ctx.redundant);
    return ctx.redundant ? 1 : 0;
}

#ifdef __cplusplus
}
#endif
