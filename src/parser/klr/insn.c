/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

static void init_use(KlrUse *use, KlrInsn *insn, KlrOper *oper, KlrValue *ref, int is_def)
{
    use->insn = insn;
    use->oper = oper;
    init_list(&use->use_link);
    use->is_def = is_def;
    if (is_def) {
        list_push_back(&ref->def_list, &use->use_link);
        ref->def_count++;
    } else {
        if (ref) {
            list_push_back(&ref->use_list, &use->use_link);
            ref->use_count++;
        }
    }
    use->ref = ref;
}

static void fini_use(KlrUse *use)
{
    list_remove(&use->use_link);

    KlrValue *ref = use->ref;

    if (use->is_def) {
        ASSERT(klr_is_local(ref));
        ref->def_count--;
        if (ref->def_count == 0) {
            log_info("erase insn '%%%s' since it has no defs after this removal", ref->name);
            ASSERT(list_empty(&ref->def_list));
            log_insn((KlrInsn *)ref);
            klr_erase_insn((KlrInsn *)ref);
        }
    } else {
        if (ref) ref->use_count--;
    }

    use->ref = NULL;
    use->insn = NULL;
    use->oper = NULL;
}

static void init_oper(KlrOper *oper, KlrInsn *insn, KlrValue *ref, int is_def)
{
    init_use(&oper->use, insn, oper, ref, is_def);
}

static void fini_oper(KlrOper *oper) { fini_use(&oper->use); }

void set_operand(KlrOper *oper, KlrInsn *insn, KlrValue *val)
{
    fini_oper(oper);

    // def-val cannot be updated, so we only update use-val.
    init_oper(oper, insn, val, 0);
}

void set_operand_at(KlrInsn *insn, int i, KlrValue *val)
{
    KlrOper *oper = insn_operand(insn, i);
    set_operand(oper, insn, val);
}

void clear_operand(KlrOper *oper) { fini_oper(oper); }

void clear_operand_at(KlrInsn *insn, int i)
{
    KlrOper *oper = insn_operand(insn, i);
    clear_operand(oper);
}

int replace_all_uses_with(KlrValue *val, KlrValue *def)
{
    int changed = 0;
    KlrUse *use, *next;
    use_foreach_safe(use, next, def) {
        KlrInsn *insn = use->insn;

        if (insn->code == OP_MOVE && use->oper == &insn->opers[0]) {
            // special case for move instruction, only replace the source
            // operand, keep the destination operand unchanged.
            continue;
        }
        set_operand(use->oper, use->insn, val);
        changed = 1;
    }
    return changed;
}

static int __attr_eq__(void *a, void *b) { return a == b; }

static KlrInsn *new_insn(OpCode op, int num_opers, char *name)
{
    KlrInsn *insn = mm_alloc(sizeof(*insn) + sizeof(KlrOper) * num_opers);
    INIT_KLR_VALUE(insn, KLR_VALUE_INSN, NULL, name);
    insn->code = op;
    insn->num_opers = num_opers;
    init_list(&insn->bb_link);
    hashmap_init(&insn->attrs, __attr_eq__);
    return insn;
}

void klr_append_insn(KlrBuilder *bldr, KlrInsn *insn)
{
    KlrBasicBlock *bb = bldr->bb;
    list_add(bldr->it, &insn->bb_link);
    insn->bb = bb;
    bldr->it = &insn->bb_link;
    ++bb->num_insns;
}

void klr_erase_insn(KlrInsn *insn)
{
    KlrBasicBlock *bb = insn->bb;
    list_remove(&insn->bb_link);
    --bb->num_insns;
    ASSERT(list_empty(&insn->use_list));
    ASSERT(insn->use_count == 0);
    ASSERT(list_empty(&insn->def_list));
    ASSERT(insn->def_count == 0);
    for (int i = 0; i < insn->num_opers; i++) {
        fini_oper(&insn->opers[i]);
    }
    mm_free(insn);
}

/* no allocate register codes */
static OpCode no_regs_codes[] = {
    OP_MOVE,
    OP_LOAD_INT_IMM,
    OP_LOADK,
    OP_LOAD_TAG,
    OP_IR_JMP_COND,
    OP_RET,
    OP_RET_INT_IMM,
    OP_RET_TAG,
    OP_RET_CONST,
    OP_RET_VOID,
    OP_JMP,
    OP_JMP_TRUE,
    OP_JMP_FALSE,
    OP_JMP_INT_EQ,
    OP_JMP_INT_NE,
    OP_JMP_INT_LT,
    OP_JMP_INT_GT,
    OP_JMP_INT_LE,
    OP_JMP_INT_GE,
    OP_JMP_INT_EQ_IMM,
    OP_JMP_INT_NE_IMM,
    OP_JMP_INT_LT_IMM,
    OP_JMP_INT_GT_IMM,
    OP_JMP_INT_LE_IMM,
    OP_JMP_INT_GE_IMM,
    OP_GLOBAL_SET,
    OP_TAIL_CALL,
    OP_SEQ_SET,
    OP_MAP_SET,
    OP_SEQ_SET_IMM,
};

int ir_has_value(KlrInsn *insn)
{
    for (int i = 0; i < COUNT_OF(no_regs_codes); i++) {
        if (insn->code == no_regs_codes[i]) return 0;
    }

    if ((insn->code == OP_IR_CALL || insn->code == OP_CALL) && type_is_no_type(insn->ts)) {
        return 0;
    }

    return 1;
}

/*
 * IR: move %dst, %src
 * %dst is a local/param variable, %src is a reg value or const value.
 * Here, %dst is allowed for parameter for tailcall optimization.
 * In koala, all parameters are immutable local variables, it's checked by front-end.
 * This is not changed for Koala language.
 */
void klr_build_move(KlrBuilder *bldr, KlrValue *var, KlrValue *val)
{
    if (!klr_is_local(var) && !klr_is_param(var)) {
        panic("'move %%x, %%v' requires a local/param var.");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_PARAM &&
        val->kind != KLR_VALUE_INSN) {
        panic("'move %%x, %%v' requires a reg value.");
    }

    // ASSERT(var->ts == val->ts);

    KlrInsn *insn = new_insn(OP_MOVE, 2, "");
    init_oper(&insn->opers[0], insn, var, 1);
    init_oper(&insn->opers[1], insn, val, 0);
    klr_append_insn(bldr, insn);
}

/*
 * IR: %0 = local int [immutable]
 *
 * create a local immutable variable of function, return the local variable as a
 * value. %0 is a local variable of function, its type is int, and it's
 * immutable.
 */
KlrValue *klr_build_local(KlrBuilder *bldr, TypeSpec *ts, char *name)
{
    KlrInsn *insn = new_insn(OP_IR_LOCAL, 0, name);
    insn->ts = ts;
    insn->flags |= KLR_INSN_FLAGS_CONST;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

/*
 * IR: %0 = local float [mutable]
 *
 * %var is a local variable of function
 */
KlrValue *klr_build_local_var(KlrBuilder *bldr, TypeSpec *ts, char *name)
{
    KlrInsn *insn = new_insn(OP_IR_LOCAL, 0, name);
    insn->ts = ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

/*
 * IR: %0 = get_global %global
 * get a global variable, %global is a global variable, return the value of this
 * global variable.
 */
KlrValue *klr_build_get_global(KlrBuilder *bldr, KlrValue *global)
{
    if (global->kind != KLR_VALUE_GLOBAL && global->kind != KLR_VALUE_EXT_GLOBAL) {
        panic("'get_global %%g' requires a global variable.");
    }

    KlrInsn *insn = new_insn(OP_GLOBAL_GET, 1, "");
    init_oper(&insn->opers[0], insn, global, 0);
    insn->ts = global->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

/*
 * IR: set_global %global, %src
 * set a global variable, %global is a global variable, %src is a reg value or
 * const
 */
void klr_build_set_global(KlrBuilder *bldr, KlrValue *global, KlrValue *val)
{
    if (global->kind != KLR_VALUE_GLOBAL && global->kind != KLR_VALUE_EXT_GLOBAL) {
        panic("'set_global %%g, %%v' requires a global variable.");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM && val->kind != KLR_VALUE_EXT_GLOBAL) {
        panic("'set_global %%g, %%v' requires a reg value.");
    }

    KlrInsn *insn = new_insn(OP_GLOBAL_SET, 2, "");
    init_oper(&insn->opers[0], insn, global, 1);
    init_oper(&insn->opers[1], insn, val, 0);
    klr_append_insn(bldr, insn);
}

KlrValue *klr_build_binary(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode op, char *name,
                           const char *op_name)
{
    if (lhs->kind != KLR_VALUE_CONST && lhs->kind != KLR_VALUE_INSN &&
        lhs->kind != KLR_VALUE_PARAM) {
        panic("'%s %%x, %%y' requires both reg vars/consts", op_name);
    }

    if (rhs->kind != KLR_VALUE_CONST && rhs->kind != KLR_VALUE_INSN &&
        rhs->kind != KLR_VALUE_PARAM) {
        panic("'%s %%x, %%y' requires both reg vars/consts", op_name);
    }

    KlrInsn *insn = new_insn(op, 2, name);
    init_oper(&insn->opers[0], insn, lhs, 0);
    init_oper(&insn->opers[1], insn, rhs, 0);
    TypeSpec *ty = lhs->ts;
    insn->ts = ty;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_unary(KlrBuilder *bldr, KlrValue *operand, OpCode op, char *name,
                          const char *op_name)
{
    if (operand->kind != KLR_VALUE_CONST && operand->kind != KLR_VALUE_INSN &&
        operand->kind != KLR_VALUE_PARAM) {
        panic("'%s %%x' requires a reg var/const", op_name);
    }

    KlrInsn *insn = new_insn(op, 1, name);
    init_oper(&insn->opers[0], insn, operand, 0);

    if (op == OP_UNARY_NEG || op == OP_UNARY_PLUS || op == OP_UNARY_NOT) {
        TypeSpec *ty = operand->ts;
        insn->ts = ty;
    } else if (op == OP_LNOT) {
        insn->ts = bool_type_spec();
    } else {
        UNREACHABLE();
    }

    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_cmp(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode code, char *name)
{
    if (lhs->kind != KLR_VALUE_CONST && lhs->kind != KLR_VALUE_INSN &&
        lhs->kind != KLR_VALUE_PARAM) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    if (rhs->kind != KLR_VALUE_CONST && rhs->kind != KLR_VALUE_INSN &&
        rhs->kind != KLR_VALUE_PARAM) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    KlrInsn *insn = new_insn(code, 2, name);
    init_oper(&insn->opers[0], insn, lhs, 0);
    init_oper(&insn->opers[1], insn, rhs, 0);
    insn->ts = bool_type_spec();
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_select(KlrBuilder *bldr, KlrValue *cond, KlrValue *true_val,
                           KlrValue *false_val, char *name)
{
    if (cond->ts->kind != TYPE_BOOL) {
        panic("'select %%cond, %%true, %%false' requires a bool cond");
    }

    if (true_val->kind != KLR_VALUE_CONST && true_val->kind != KLR_VALUE_INSN &&
        true_val->kind != KLR_VALUE_PARAM) {
        panic("'select %%cond, %%true, %%false' requires reg/const for true_val");
    }

    if (false_val->kind != KLR_VALUE_CONST && false_val->kind != KLR_VALUE_INSN &&
        false_val->kind != KLR_VALUE_PARAM) {
        panic("'select %%cond, %%true, %%false' requires reg/const for false_val");
    }

    if (true_val->ts != false_val->ts) {
        panic(
            "'select %%cond, %%true, %%false' requires true_val and false_val to have "
            "the same type");
    }

    KlrInsn *insn = new_insn(OP_IR_SELECT, 3, name);
    init_oper(&insn->opers[0], insn, cond, 0);
    init_oper(&insn->opers[1], insn, true_val, 0);
    init_oper(&insn->opers[2], insn, false_val, 0);
    insn->ts = true_val->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_jmp_cond(KlrBuilder *bldr, KlrValue *cond, KlrBasicBlock *_then,
                        KlrBasicBlock *_else)
{
    if (cond->ts->kind != TYPE_BOOL) {
        panic("'branch %%cond, %%b1, %%b2' requires a bool cond");
    }

    KlrInsn *insn = new_insn(OP_IR_JMP_COND, 4, "");
    init_oper(&insn->opers[0], insn, cond, 0);
    // reserved for jmp_cond_fused
    init_oper(&insn->opers[1], insn, NULL, 0);
    init_oper(&insn->opers[2], insn, (KlrValue *)_then, 0);
    init_oper(&insn->opers[3], insn, (KlrValue *)_else, 0);
    klr_append_insn(bldr, insn);

    klr_link_edge(bldr->bb, _then);
    klr_link_edge(bldr->bb, _else);
}

void klr_build_jmp(KlrBuilder *bldr, KlrBasicBlock *target)
{
    KlrInsn *insn = new_insn(OP_JMP, 1, "");
    init_oper(&insn->opers[0], insn, (KlrValue *)target, 0);
    klr_append_insn(bldr, insn);

    klr_link_edge(bldr->bb, target);
}

KlrValue *klr_build_call(KlrBuilder *bldr, KlrValue *fn, TypeSpec *ret, KlrValue **args, int nargs,
                         char *name)
{
    int is_const = 1;

    if (nargs <= 0) is_const = 0;

    for (int i = 0; i < nargs; i++) {
        if (!klr_is_const(args[i])) {
            is_const = 0;
            break;
        }
    }

    KlrInsn *insn = new_insn(OP_IR_CALL, nargs + 1, name);
    insn->flags |= is_const ? KLR_INSN_FLAGS_CONST : 0;

    ASSERT(fn);
    init_oper(&insn->opers[0], insn, (KlrValue *)fn, 0);
    for (int j = 0; j < nargs; j++) {
        init_oper(&insn->opers[j + 1], insn, (KlrValue *)args[j], 0);
    }
    insn->ts = ret ? ret : fn->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_ret(KlrBuilder *bldr, KlrValue *ret)
{
    KlrInsn *insn = new_insn(OP_RET, 1, "");
    init_oper(&insn->opers[0], insn, ret, 0);
    klr_append_insn(bldr, insn);

    KlrFunc *fn = bldr->bb->func;
    klr_link_edge(bldr->bb, fn->ebb);
}

void klr_build_ret_void(KlrBuilder *bldr)
{
    KlrInsn *insn = new_insn(OP_RET_VOID, 0, "");
    klr_append_insn(bldr, insn);

    KlrFunc *fn = bldr->bb->func;
    klr_link_edge(bldr->bb, fn->ebb);
}

KlrInsn *klr_build_phi(KlrBasicBlock *bb, KlrValue *var, char *name)
{
    if (!klr_is_local(var)) {
        panic("'phi' op requires a local var");
    }

    KlrInsn *_var = (KlrInsn *)var;
    if (_var->flags & KLR_INSN_FLAGS_CONST) {
        panic("'phi' op requires a mutable local var");
    }

    int num_preds = klr_get_nr_preds(bb);
    KlrInsn *insn = new_insn(OP_IR_PHI, num_preds, name);
    insn->ts = var->ts;
    /* the original variable whose SSA versions are merged by this phi */
    insn->target = var;

    insn->phi_preds = mm_alloc(sizeof(KlrBasicBlock *) * num_preds);

    KlrBuilder bldr;
    klr_builder_head(&bldr, bb);

    klr_append_insn(&bldr, insn);
    return insn;
}

void klr_append_phi_operand(KlrInsn *phi, KlrValue *val, KlrBasicBlock *pred)
{
    if (phi->code != OP_IR_PHI) {
        panic("'append_phi_operand' requires a phi insn");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'append_phi_operand' requires a reg value or const");
    }

    ASSERT(phi->filled < phi->num_opers);
    KlrOper *oper = phi->opers + phi->filled++;
    init_oper(oper, phi, val, 0);

    ASSERT(phi->phi_preds);
    phi->phi_preds[phi->filled - 1] = pred;
}

/**
 * Insert a move instruction at the logical exit point of a predecessor block.
 * Shared by PHI Coalescing (rewrite) and De-SSA passes.
 *
 * Handles conditional branches correctly: if the terminator is OP_IR_JMP_COND
 * and its condition is defined in the same block, the move is inserted BEFORE
 * the condition instruction to preserve Def-Use integrity.
 *
 * bb: Target predecessor basic block
 * dst: Move destination (local slot)
 * src: Move source (SSA value)
 *
 */
void klr_build_move_before_terminator(KlrBasicBlock *bb, KlrValue *dst, KlrValue *src)
{
    /* Universal self-assignment filter */
    ASSERT(dst != src && "Self-assignment detected");
    ASSERT(bb && dst && src);
    ASSERT(klr_is_local(dst) && "Destination must be a local variable");

    KlrInsn *last = insn_last(bb);
    ASSERT(insn_is_terminator(last));

    KlrBuilder bldr;

    if (last->code == OP_IR_JMP_COND) {
        /* Conditional branch: condition may be defined in this block.
         * Insert move BEFORE the condition instruction to avoid
         * clobbering the condition value's live range. */
        KlrValue *cond = insn_oper_value(last, 0);
        if (klr_is_insn(cond)) {
            klr_builder_before(&bldr, (KlrInsn *)cond);
        } else {
            /* Condition is a param/constant — safe to insert before terminator */
            klr_builder_before(&bldr, last);
        }
    } else {
        /* Unconditional jump / return: insert directly before terminator */
        klr_builder_before(&bldr, last);
    }

    klr_build_move(&bldr, dst, src);
}

KlrInsn *klr_build_push(KlrBuilder *bldr, KlrValue *val, OpCode op)
{
    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'push' op requires a reg value or const");
    }

    KlrInsn *insn = new_insn(op, 1, "");
    init_oper(&insn->opers[0], insn, val, 0);
    klr_append_insn(bldr, insn);
    return insn;
}

KlrInsn *klr_build_load(KlrBuilder *bldr, KlrValue *var, KlrValue *val, OpCode op)
{
    if (!klr_is_local(var) && !klr_is_param(var)) {
        panic("'load' op requires a local/param var");
    }

    if (val->kind != KLR_VALUE_CONST) {
        panic("'load' op requires a const value");
    }

    KlrInsn *insn = new_insn(op, 2, "");
    init_oper(&insn->opers[0], insn, var, 0);
    init_oper(&insn->opers[1], insn, val, 0);
    insn->ts = val->ts;
    klr_append_insn(bldr, insn);
    return insn;
}

KlrValue *klr_build_cast(KlrBuilder *bldr, KlrValue *val, TypeSpec *dst_ts, char *name)
{
    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'ir_cast' op requires a reg value or const");
    }

    // ASSERT(!type_is_optional(val->ts));
    ASSERT(!type_is_optional(dst_ts));
    ASSERT(val->ts != dst_ts);

    KlrInsn *insn = new_insn(OP_IR_CAST, 1, name);
    init_oper(&insn->opers[0], insn, val, 0);
    insn->ts = dst_ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_new(KlrBuilder *bldr, KlrValue *klass, TypeSpec *ts, char *name)
{
    if (klass->kind != KLR_VALUE_KLASS && klass->kind != KLR_VALUE_EXT_KLASS) {
        panic("'new' op requires a klass value");
    }

    KlrInsn *insn = new_insn(OP_NEW, 1, name);
    init_oper(&insn->opers[0], insn, klass, 0);
    insn->ts = ts ? ts : klass->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_make_intf(KlrBuilder *bldr, KlrValue *val, TypeSpec *dst_ts, int intf_index,
                              char *name)
{
    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'make_intf' op requires a reg value or const");
    }

    // ASSERT(!type_is_optional(val->ts));
    ASSERT(!type_is_optional(dst_ts));

    KlrInsn *insn = new_insn(OP_MAKE_INTF, 1, name);
    init_oper(&insn->opers[0], insn, val, 0);
    insn->ts = dst_ts;
    set_raw_imm(&insn->raws[2], intf_index);
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_upcast_intf(KlrBuilder *bldr, KlrValue *val, TypeSpec *dst_ts, int intf_index,
                                char *name)
{
    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'upcast_intf' op requires a reg value or const");
    }

    // ASSERT(!type_is_optional(val->ts));
    ASSERT(!type_is_optional(dst_ts));

    KlrInsn *insn = new_insn(OP_UPCAST_INTF, 1, name);
    init_oper(&insn->opers[0], insn, val, 0);
    insn->ts = dst_ts;
    set_raw_imm(&insn->raws[2], intf_index);
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_intern(KlrBuilder *bldr, KlrValue **args, int nargs, TypeSpec *ts,
                           InternTag tag, char *name)
{
    int is_const = 1;

    if (nargs <= 0) is_const = 0;

    for (int i = 0; i < nargs; i++) {
        if (!klr_is_const(args[i])) {
            is_const = 0;
            break;
        }
    }

    KlrInsn *insn = new_insn(OP_BUILD_INTERN, nargs, name);
    insn->flags |= is_const ? KLR_INSN_FLAGS_CONST : 0;

    for (int j = 0; j < nargs; j++) {
        init_oper(&insn->opers[j], insn, args[j], 0);
    }

    insn->ts = ts;
    insn->intern_tag = tag;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_get_field(KlrBuilder *bldr, KlrValue *obj, KlrValue *field, TypeSpec *ts,
                              char *name)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM) {
        panic("'get_field' op requires a reg/param value");
    }

    KlrInsn *insn = new_insn(OP_GET_FIELD, 2, name);
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, field, 0);

    insn->ts = ts ? ts : field->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_get_field_ext(KlrBuilder *bldr, KlrValue *obj, KlrValue *field, char *name)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM) {
        panic("'get_field' op requires a reg/param value");
    }

    KlrInsn *insn = new_insn(OP_GET_FIELD_EXT, 2, name);
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, field, 0);

    insn->ts = field->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_set_field(KlrBuilder *bldr, KlrValue *obj, KlrValue *field, KlrValue *val)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM) {
        panic("'set_field' op requires a reg/param value for obj");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'set_field' op requires a reg/param value for val");
    }

    KlrInsn *insn = new_insn(OP_SET_FIELD, 3, "");
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, field, 0);
    init_oper(&insn->opers[2], insn, val, 0);

    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_set_field_ext(KlrBuilder *bldr, KlrValue *obj, KlrValue *field, KlrValue *val)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM) {
        panic("'set_field' op requires a reg/param value for obj");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'set_field' op requires a reg/param value for val");
    }

    KlrInsn *insn = new_insn(OP_SET_FIELD_EXT, 3, "");
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, field, 0);
    init_oper(&insn->opers[2], insn, val, 0);

    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_seq_get(KlrBuilder *bldr, KlrValue *obj, KlrValue *index, TypeSpec *ts,
                            char *name)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM &&
        obj->kind != KLR_VALUE_CONST) {
        panic("'seq_get' op requires a reg/param/const value for obj");
    }

    if (index->kind != KLR_VALUE_CONST && index->kind != KLR_VALUE_INSN &&
        index->kind != KLR_VALUE_PARAM) {
        panic("'index_get' op requires a reg/param/const value for index");
    }

    KlrInsn *insn = new_insn(OP_SEQ_GET, 2, name);
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, index, 0);

    insn->ts = ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_seq_set(KlrBuilder *bldr, KlrValue *obj, KlrValue *index, KlrValue *val)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM &&
        obj->kind != KLR_VALUE_CONST) {
        panic("'seq_set' op requires a reg/param/const value for obj");
    }

    if (index->kind != KLR_VALUE_CONST && index->kind != KLR_VALUE_INSN &&
        index->kind != KLR_VALUE_PARAM) {
        panic("'seq_set' op requires a reg/param/const value for index");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'seq_set' op requires a reg/param/const value for val");
    }

    KlrInsn *insn = new_insn(OP_SEQ_SET, 3, "");
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, index, 0);
    init_oper(&insn->opers[2], insn, val, 0);

    klr_append_insn(bldr, insn);
}

KlrValue *klr_build_seq_len(KlrBuilder *bldr, KlrValue *obj, char *name)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM &&
        obj->kind != KLR_VALUE_CONST) {
        panic("'seq_len' op requires a reg/param/const value for obj");
    }

    KlrInsn *insn = new_insn(OP_SEQ_LEN, 1, name);
    init_oper(&insn->opers[0], insn, obj, 0);

    insn->ts = int64_type_spec();
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_map_get(KlrBuilder *bldr, KlrValue *obj, KlrValue *index, TypeSpec *ts,
                            char *name)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM &&
        obj->kind != KLR_VALUE_CONST) {
        panic("'map_get' op requires a reg/param/const value for obj");
    }

    if (index->kind != KLR_VALUE_CONST && index->kind != KLR_VALUE_INSN &&
        index->kind != KLR_VALUE_PARAM) {
        panic("'map_get' op requires a reg/param/const value for index");
    }

    KlrInsn *insn = new_insn(OP_MAP_GET, 2, name);
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, index, 0);

    insn->ts = ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_map_set(KlrBuilder *bldr, KlrValue *obj, KlrValue *index, KlrValue *val)
{
    if (obj->kind != KLR_VALUE_INSN && obj->kind != KLR_VALUE_PARAM &&
        obj->kind != KLR_VALUE_CONST) {
        panic("'map_set' op requires a reg/param/const value for obj");
    }

    if (index->kind != KLR_VALUE_CONST && index->kind != KLR_VALUE_INSN &&
        index->kind != KLR_VALUE_PARAM) {
        panic("'map_set' op requires a reg/param/const value for index");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
        panic("'map_set' op requires a reg/param/const value for val");
    }

    KlrInsn *insn = new_insn(OP_MAP_SET, 3, "");
    init_oper(&insn->opers[0], insn, obj, 0);
    init_oper(&insn->opers[1], insn, index, 0);
    init_oper(&insn->opers[2], insn, val, 0);

    klr_append_insn(bldr, insn);
}

#ifdef __cplusplus
}
#endif
