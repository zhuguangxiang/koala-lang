/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
#include "cmd.h"
#include "ir.h"
#include "log.h"
#include "opt.h"

/**
 * Koala SSA construction based on:
 * Braun et al., "Simple and Efficient Construction of Static Single Assignment Form"
 * CGO 2013
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CurrentDef {
    /* hashmap entry */
    HashMapEntry hnode;

    /* key: original variable */
    KlrValue *key;

    /* value: current SSA version (phi or normal insn) */
    KlrValue *value;
} CurrentDef;

static int _phi_curent_def_equal_(void *a, void *b)
{
    CurrentDef *ua = (CurrentDef *)a;
    CurrentDef *ub = (CurrentDef *)b;
    return ua->key == ub->key;
}

static void init_phi_current_defs(HashMap *map) { hashmap_init(map, _phi_curent_def_equal_); }

static void _phi_current_def_free_(void *entry, void *arg)
{
    CurrentDef *e = (CurrentDef *)entry;
    e->key = NULL;
    e->value = NULL;
    mm_free(e);
}

static void fini_phi_current_defs(HashMap *map)
{
    hashmap_fini(map, _phi_current_def_free_, NULL);
}

/**
 * Checks whether a PHI instruction is incomplete (i.e., its currently populated
 * operands count is less than the basic block's physical in-edges count).
 * Incomplete PHIs represent active loop back-edge headers during Phase 1.
 */
static inline int phi_is_incomplete(KlrInsn *phi)
{
    ASSERT(phi->code == OP_IR_PHI);
    return phi->filled < phi->num_opers;
}

/* Write the SSA version 'v' for original variable 'x' in block 'bb' */
static void write_variable(KlrBasicBlock *bb, KlrValue *x, KlrValue *v)
{
    CurrentDef *entry = mm_alloc_obj(entry);
    hashmap_entry_init(entry, mem_hash(&x, sizeof(x)));
    entry->key = x;
    entry->value = v;
    hashmap_put(&bb->current_defs, entry);
    log_info("write_variable: %s -> %s in %%bb%d", klr_value_name(x), klr_value_name(v), bb->tag);
}

/* Forward declaration */
static KlrValue *read_variable_recursive(KlrBasicBlock *bb, KlrValue *x);

/* Read the current SSA version of variable 'x' in block 'bb' */
static KlrValue *read_variable(KlrBasicBlock *bb, KlrValue *x)
{
    /* Construct a temporary key for lookup */
    CurrentDef key = { .key = x };
    hashmap_entry_init(&key, mem_hash(&x, sizeof(x)));

    /* Try to read from CurrentDefs[B] */
    CurrentDef *entry = hashmap_get(&bb->current_defs, &key);
    if (entry) {
        log_info("read_variable: %s -> %s in %%bb%d", klr_value_name(x),
                 klr_value_name(entry->value), bb->tag);
        return entry->value;
    }

    /* Otherwise, fall back to recursive lookup */
    return read_variable_recursive(bb, x);
}

static KlrValue *try_remove_trivial_phi(KlrInsn *phi)
{
    ASSERT(phi->code == OP_IR_PHI);

    if (phi_is_incomplete(phi)) {
        // The PHI is not fully populated yet; cannot determine triviality.
        log_info(
            "try_remove_trivial_phi: PHI in %%bb%d for variable %s is incomplete (%db filled, %db "
            "total), skipping triviality check",
            phi->bb->tag, klr_value_name(phi->target), phi->filled, phi->num_opers);
        return (KlrValue *)phi;
    }

    if (phi->flags & KLR_INSN_FLAGS_VISITED) {
        // Already visited, avoid infinite recursion
        return (KlrValue *)phi;
    }

    phi->flags |= KLR_INSN_FLAGS_VISITED; // mark as visited to avoid infinite recursion

    KlrValue *replacement = NULL;

    /* Step 1: find a unique non-self operand
     * Using phi->filled as the boundary constraint ensures safe partial evaluation
     * and avoids loading unpopulated operand slots during early recursive sweeps.
     */
    for (int i = 0; i < phi->filled; i++) {
        KlrValue *v = insn_oper_value(phi, i);

        // Ignore self-references (phi itself)
        if ((KlrInsn *)v == phi) continue;

        if (!replacement) {
            replacement = v;
        } else if (replacement != v) {
            phi->flags &= ~KLR_INSN_FLAGS_VISITED;
            return (KlrValue *)phi; // non-trivial
        }
    }

    // All operands were self-references -> not trivial
    if (!replacement) return (KlrValue *)phi;

    // Step 2: replace all uses of phi with the replacement value
    replace_all_uses_with(replacement, (KlrValue *)phi);

    // Step 3: erase the phi instruction physically from the KLR CFG stream
    // klr_erase_insn(phi);
    phi->flags |= KLR_INSN_FLAGS_DEAD;

    // Step 4: cascade: any phi that uses 'replacement' may now become trivial
    KlrUse *use, *next;
    use_foreach_safe(use, next, replacement) {
        KlrInsn *insn = use->insn;
        if (insn->code == OP_IR_PHI) try_remove_trivial_phi(insn);
    }

    return replacement;
}

static KlrValue *add_phi_operands(KlrValue *x, KlrInsn *phi, KlrBasicBlock *bb)
{
    /* Forward-edge boundary protection:
     * Only recursively look up the variable if the predecessor precedes the
     * current block in the RPO sequence (i.e., a standard forward or non-loop edge).
     * If pred->index >= bb->index, it is a loop back-edge (including self-loops).
     * Skip it cleanly during Phase 1; it will be populated during Phase 2.
     * This strictly prevents the PHI node from collapsing into a constant prematurely
     * before the loop body has been fully processed.
     */
    KlrEdge *in_edge;
    edge_in_foreach(in_edge, bb) {
        KlrBasicBlock *pred = in_edge->src;

        if (pred->index >= bb->index) {
            log_info("add_phi_operands: skipping back-edge from %%bb%d to %%bb%d for variable %s",
                     pred->tag, bb->tag, klr_value_name(x));
            continue;
        }

        KlrValue *incoming = read_variable(pred, x);
        log_info("add_phi_operands: appending %s from %%bb%d to PHI in %%bb%d for variable %s",
                 klr_value_name(incoming), pred->tag, bb->tag, klr_value_name(x));
        klr_append_phi_operand(phi, incoming, pred);
    }

    if (phi_is_incomplete(phi)) {
        // If the PHI is still incomplete (due to skipped back-edges), return it as-is for now.
        log_info(
            "add_phi_operands: PHI in %%bb%d for variable %s is still incomplete (%db filled, %db "
            "total), deferring triviality check",
            bb->tag, klr_value_name(x), phi->filled, phi->num_opers);
        return (KlrValue *)phi;
    }

    /* Since loop back-edges are skipped, the PHI operand list is still incomplete.
     * This mathematically guarantees that the PHI cannot be falsely detected as trivial
     * and will be safely preserved on the graph during Phase 1.
     */
    return try_remove_trivial_phi(phi);
}

/* Recursive lookup of variable 'x' through predecessors of block 'bb' */
static KlrValue *read_variable_recursive(KlrBasicBlock *bb, KlrValue *x)
{
    /* Case 1: no predecessors → x is undefined or a parameter */
    if (bb->num_inedges == 0) {
        /* Return the original variable as its own SSA version */
        log_info("read_variable_recursive: %s -> %s in %%bb%d (no predecessors)",
                 klr_value_name(x), klr_value_name(x), bb->tag);
        return x;
    }

    /* Case 2: multiple predecessors → need a phi */
    if (bb->num_inedges > 1) {
        /* Create a phi instruction for x */
        ASSERT(klr_is_insn(x));
        KlrInsn *_x = (KlrInsn *)x;
        if (_x->def_count <= 1) {
            log_info("variable is only one set. No need to create phi for %s in %%bb%d",
                     klr_value_name(x), bb->tag);
            return x;
        }

        char buf[64];
        int n = snprintf(buf, sizeof(buf), "%s.phi.%d", _x->name, _x->phi_index++);
        KlrInsn *phi = klr_build_phi(bb, x, atom_nstr(buf, n));

        /* Strictly following paper: Immediately cache the PHI in the current block's
         * map BEFORE evaluating predecessors. This functions as a "memoization barrier"
         * that intercepts cyclic self-references and prevents infinite recursion stack overflow.
         */
        write_variable(bb, x, (KlrValue *)phi);

        /* Populate the PHI immediately with ALL its predecessors */
        KlrValue *final_val = add_phi_operands(x, phi, bb);

        /* Overwrite and update the map with the finalized collapsed version */
        write_variable(bb, x, final_val);

        log_info("read_variable_recursive: %s -> %s in %%bb%d (phi)", klr_value_name(x),
                 klr_value_name(final_val), bb->tag);
        return final_val;
    }

    /* Case 3: exactly one predecessor → forward the version */
    KlrBasicBlock *pred = edge_in_first(bb)->src;
    KlrValue *v = read_variable(pred, x);

    /* Cache the result in CurrentDefs[B] */
    write_variable(bb, x, v);

    log_info("read_variable_recursive: %s -> %s in %%bb%d (single predecessor)", klr_value_name(x),
             klr_value_name(v), bb->tag);
    return v;
}

static int phi_operand_is_from_pred(KlrInsn *phi, KlrBasicBlock *pred)
{
    /* Scrutinize currently filled slots to verify if an operand from 'pred' already exists */
    for (int i = 0; i < phi->filled; i++) {
        KlrValue *val = insn_oper_value(phi, i);
        if (val && val->kind == KLR_VALUE_INSN && ((KlrInsn *)val)->bb == pred) {
            return 1;
        }
    }
    return 0;
}

static void build_ssa(KlrFunc *func)
{
    // Step 1: build rpo
    klr_build_rpo(func);

    // Step 2: initialize each basic block's currentDef map
    KlrBasicBlock *bb;
    basic_block_foreach(bb, func) {
        init_phi_current_defs(&bb->current_defs);
    }

    // Step 3: SSA construction

    // Phase 1 - Forward Propagation & Short-circuiting
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        KlrInsn *next;
        insn_foreach_safe(insn, next, bb) {
            if (insn->code == OP_IR_PHI) {
                // For phi instructions, we don't process operands here
                continue;
            }

            if (insn->code == OP_MOVE) {
                KlrValue *dst = insn_oper_value(insn, 0);
                KlrValue *src = insn_oper_value(insn, 1);

                if (klr_is_local(src)) {
                    KlrValue *v = read_variable(bb, src);
                    set_operand_at(insn, 1, v);
                    src = v;
                }

                ASSERT(klr_is_local(dst));
                write_variable(bb, dst, src);
                // klr_erase_insn(insn);
                continue;
            }

            for (int i = 0; i < insn->num_opers; i++) {
                if (insn->code == OP_IR_JMP_COND) {
                    // skip the label operand(basic block) for conditional jump
                    if (i >= 1) continue;
                }

                KlrValue *x = insn_oper_value(insn, i);
                ASSERT(x);
                /* Trigger promotion only if the referenced value is a non-SSA
                 * mutable local variable
                 */
                if (klr_is_local(x)) {
                    KlrInsn *_x = (KlrInsn *)x;
                    if (!(_x->flags & KLR_INSN_FLAGS_CONST)) {
                        KlrValue *v = read_variable(bb, x);
                        set_operand_at(insn, i, v);
                    }
                }
            }
        }
    }

    // Phase 2 - Back-filling: Complete Loop Phi Operands
    // Now that Phase 1 has concluded globally, all final variable definitions
    // have settled inside their respective basic block maps (including loop bodies).
    // We traverse all generated PHI nodes to resolve and append the remaining
    // loop back-edge arguments point-to-point.
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn->code != OP_IR_PHI) break;

            if (insn->filled >= insn->num_opers) {
                continue;
            }

            KlrValue *local_var = insn->target;

            KlrEdge *in_edge;
            edge_in_foreach(in_edge, bb) {
                KlrBasicBlock *pred = in_edge->src;
                /* Capture and fill loop back-edges skipped during Phase 1 */
                if (pred->index >= bb->index) {
                    log_info("build_ssa: filling back-edge from %%bb%d to %%bb%d for variable %s",
                             pred->tag, bb->tag, klr_value_name(local_var));
                    CurrentDef key = { .key = local_var };
                    hashmap_entry_init(&key, mem_hash(&local_var, sizeof(local_var)));
                    CurrentDef *entry = hashmap_get(&pred->current_defs, &key);
                    ASSERT(entry && entry->value);
                    KlrValue *final_pred_val = entry->value;

                    /* Append the correct cyclic SSA definition into the PHI node slots */
                    log_info(
                        "build_ssa: appending %s from %%bb%d to PHI in %%bb%d for variable %s",
                        klr_value_name(final_pred_val), pred->tag, bb->tag,
                        klr_value_name(local_var));
                    klr_append_phi_operand(insn, final_pred_val, pred);
                } else {
                    log_info(
                        "build_ssa: skipping already filled edge from %%bb%d to %%bb%d for "
                        "variable %s",
                        pred->tag, bb->tag, klr_value_name(local_var));
                }
            }
        }
    }

    // Step 4: Try to remove trivial phi instructions (Global Cascading Sweep)
    // All PHI arguments are now completely filled. We run the cascading trivial
    // phi elimination pass to smash and collapse any redundant phi operations globally.
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        KlrInsn *next;
        insn_foreach_safe(insn, next, bb) {
            if (insn->code == OP_IR_PHI) {
                try_remove_trivial_phi(insn);
            }
        }
    }

    // Step 5: clean up
    basic_block_foreach(bb, func) {
        fini_phi_current_defs(&bb->current_defs);
    }
}

void kl_do_ssa(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
        build_ssa(fn);
        if (dump_ssa_enabled()) {
            fprintf(stdout, "--- IR Dump After ssa [@%s] ---\n", fn->name);
            klr_print_func(fn, stdout);
        }
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            build_ssa(fn);
            if (dump_ssa_enabled()) {
                fprintf(stdout, "--- IR Dump After ssa [@%s::%s] ---\n", kls->name, fn->name);
                klr_print_func(fn, stdout);
            }
        }
    }
}

/**
 * De-SSA (PHI Elimination) for Koala KLR IR.
 * This version exclusively handles Local Op allocation, universal use remapping,
 * and predecessor copy insertion. The redundant PHI instructions are intentionally
 * preserved on the graph to be automatically swept away by the subsequent DCE pass.
 */
static void exit_ssa(KlrFunc *func)
{
    ASSERT(func->sbb->num_outedges == 1);
    KlrEdge *edge = edge_out_first(func->sbb);

    KlrBasicBlock *entry_bb = edge->dst;

    KlrBuilder bldr;
    klr_builder_head(&bldr, entry_bb);

    KlrBasicBlock *bb;

    /*
     * Step 1: Create all new local ops inside the Entry Basic Block
     */
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn->code != OP_IR_PHI) break;

            char buf[64];
            int n = snprintf(buf, sizeof(buf), "%s_", insn->name);

            /* Construct a traditional mutable local variable using the PHI register's name.
             * This local op descriptor will permanently replace the abstract single-assignment
             * register. */
            KlrValue *local_op = klr_build_local_var(&bldr, insn->ts, atom_nstr(buf, n));

            /* Temporarily attach this newly minted local op onto the phi descriptor's target
             * field so subsequent remapping and copy insertion stages can cross-reference it
             * instantly. */
            insn->target = local_op;
        }
    }

    /*
     * Step 2: Replace all active uses of PHI with the local op.
     * Enforcing this BEFORE inserting copy moves decouples the def-use network.
     * This isolates the un-promoted copies from stepping onto each other's live footprints.
     */
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn->code != OP_IR_PHI) break;
            KlrValue *local_op = insn->target;

            /* Universal Use Remapping:
             * Remap every single dependent consumer instruction's operand slot
             * from referencing this abstract PHI node to referencing the clean local op.
             * This operation leaves the PHI node's use_list completely empty, rendering it dead.
             */
            replace_all_uses_with(local_op, (KlrValue *)insn);
        }
    }

    /*
     * Step 3: Insert physical OP_MOVE statements in all predecessor blocks
     */
    basic_block_foreach(bb, func) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (insn->code != OP_IR_PHI) break;
            KlrValue *local_op = insn->target;

            /* Loop over all valid filled predecessor paths to drop the corresponding version
             * copies */
            for (int i = 0; i < insn->filled; i++) {
                KlrValue *incoming_ssa_val = insn_oper_value(insn, i);
                KlrBasicBlock *pred_bb = insn->phi_preds[i];

                /* Fetch the basic block terminator (e.g., branch or jump) located at the tail
                 */
                KlrInsn *last = insn_last(pred_bb);
                ASSERT(insn_is_terminator(last));

                /* Create a raw KLR physical assignment instruction: Move local_op,
                 * incoming_ssa_val
                 */
                KlrBuilder _bldr;
                if (last->code == OP_IR_JMP_COND) {
                    KlrValue *cond = insn_oper_value(last, 0);
                    if (klr_is_insn(cond)) {
                        KlrInsn *_cond = (KlrInsn *)cond;
                        klr_builder_before(&_bldr, _cond);
                    } else {
                        klr_builder_before(&_bldr, last);
                    }
                } else {
                    klr_builder_before(&_bldr, last);
                }
                klr_build_move(&_bldr, local_op, incoming_ssa_val);
            }
        }
    }

    // remove all PHI instructions from the function's basic blocks
    basic_block_foreach(bb, func) {
        KlrInsn *insn, *nxt;
        insn_foreach_safe(insn, nxt, bb) {
            if (insn->code != OP_IR_PHI) break;
            klr_erase_insn(insn);
        }
    }
}

void kl_exit_ssa(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    KlrFunc *fn;
    func_foreach(fn, m) {
        exit_ssa(fn);
        if (dump_ssa_enabled()) {
            fprintf(stdout, "--- IR Dump After de-ssa [@%s] ---\n", fn->name);
            klr_print_func(fn, stdout);
        }
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            exit_ssa(fn);
            if (dump_ssa_enabled()) {
                fprintf(stdout, "--- IR Dump After de-ssa [@%s] ---\n", fn->name);
                klr_print_func(fn, stdout);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
