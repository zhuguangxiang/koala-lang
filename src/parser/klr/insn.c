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
    oper->phi = NULL;
}

static void fini_oper(KlrOper *oper)
{
    fini_use(&oper->use);
    oper->phi = NULL;
}

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

    ASSERT(var->ts == val->ts);

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
    if (global->kind != KLR_VALUE_GLOBAL) {
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
    if (global->kind != KLR_VALUE_GLOBAL) {
        panic("'set_global %%g, %%v' requires a global variable.");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN &&
        val->kind != KLR_VALUE_PARAM) {
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

KlrValue *klr_build_call(KlrBuilder *bldr, KlrValue *fn, KlrValue **args, int nargs, char *name)
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

    init_oper(&insn->opers[0], insn, (KlrValue *)fn, 0);
    for (int j = 0; j < nargs; j++) {
        init_oper(&insn->opers[j + 1], insn, (KlrValue *)args[j], 0);
    }
    insn->ts = fn->ts;
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
        panic("'int_cast' op requires a reg value or const");
    }

    KlrInsn *insn = new_insn(OP_IR_CAST, 1, name);
    init_oper(&insn->opers[0], insn, val, 0);
    insn->ts = dst_ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_new(KlrBuilder *bldr, KlrValue *klass, KlrValue **args, int nargs, char *name)
{
    if (klass->kind != KLR_VALUE_KLASS) {
        panic("'new' op requires a klass value");
    }

    int is_const = 1;

    if (nargs <= 0) is_const = 0;

    for (int i = 0; i < nargs; i++) {
        if (!klr_is_const(args[i])) {
            is_const = 0;
            break;
        }
    }

    KlrInsn *insn = new_insn(OP_NEW, nargs + 1, name);
    insn->flags |= is_const ? KLR_INSN_FLAGS_CONST : 0;

    init_oper(&insn->opers[0], insn, klass, 0);
    for (int j = 0; j < nargs; j++) {
        init_oper(&insn->opers[j + 1], insn, args[j], 0);
    }
    insn->ts = klass->ts;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

#ifdef __cplusplus
}
#endif
