/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

static void init_use(KlrUse *use, KlrInsn *insn, KlrOper *oper, KlrValue *ref)
{
    use->insn = insn;
    use->oper = oper;
    init_list(&use->use_link);
    list_push_back(&ref->use_list, &use->use_link);
    use->ref = ref;
}

static void fini_use(KlrUse *use) { list_remove(&use->use_link); }

static void init_oper(KlrOper *oper, KlrInsn *insn, KlrValue *ref)
{
    init_use(&oper->use, insn, oper, ref);
    oper->phi.bb = NULL;
    init_list(&oper->phi.bb_link);
    static KlrOperKind mappings[KLR_VALUE_MAX] = {
        KLR_OPER_NONE,  KLR_OPER_CONST, KLR_OPER_GLOBAL, KLR_OPER_FUNC,
        KLR_OPER_BLOCK, KLR_OPER_PARAM, KLR_OPER_LOCAL,  KLR_OPER_INSN,
    };

    if (ref->kind > KLR_VALUE_NONE && ref->kind < KLR_VALUE_MAX) {
        oper->kind = mappings[ref->kind];
    } else {
        panic("Invalid kind %d of Value", ref->kind);
    }
}

static void fini_oper(KlrOper *oper)
{
    if (oper->kind != KLR_OPER_PHI) {
        fini_use(&oper->use);
    } else {
        NIY();
    }
    oper->kind = KLR_OPER_NONE;
}

void invalidate_insn_operand(KlrInsn *insn, int i)
{
    KlrOper *oper = insn_operand(insn, i);
    KlrValue *ref = oper->use.ref;
    fini_oper(oper);
    if (!klr_value_used(ref)) {
        printf("delete unused value\n");
        if (ref->kind == KLR_VALUE_INSN) {
            printf("delete unused instruction\n");
            klr_delete_insn((KlrInsn *)ref);
        }
    }
}

void update_insn_operand(KlrInsn *insn, int i, KlrValue *val)
{
    KlrOper *oper = insn_operand(insn, i);
    if (oper->kind != KLR_OPER_NONE) {
        invalidate_insn_operand(insn, i);
    }
    init_oper(oper, insn, val);
}

static KlrInsn *new_insn(OpCode op, int num_opers, char *name)
{
    KlrInsn *insn = mm_alloc(sizeof(*insn) + sizeof(KlrOper) * num_opers);
    INIT_KLR_VALUE(insn, KLR_VALUE_INSN, NULL, name);
    insn->code = op;
    insn->num_opers = num_opers;
    init_list(&insn->bb_link);
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

void klr_delete_insn(KlrInsn *insn)
{
    KlrBasicBlock *bb = insn->bb;
    list_remove(&insn->bb_link);
    --bb->num_insns;
    ASSERT(list_empty(&insn->use_list));
    for (int i = 0; i < insn->num_opers; i++) {
        fini_oper(&insn->opers[i]);
    }
    mm_free(insn);
}

/* no allocate register codes */
static OpCode no_regs_codes[] = {
    OP_IR_STORE,
    OP_IR_JMP_COND,
    OP_PUSH,
    OP_POP,
    OP_PUSH_NONE,
    OP_PUSH_IMM8,
    OP_RETURN,
    OP_RETURN_NONE,
    OP_JMP,
    OP_JMP_TRUE,
    OP_JMP_FALSE,
    OP_JMP_NONE,
    OP_JMP_NOT_NONE,
    OP_JMP_CMP_EQ,
    OP_JMP_CMP_NE,
    OP_JMP_CMP_LT,
    OP_JMP_CMP_GT,
    OP_JMP_CMP_LE,
    OP_JMP_CMP_GE,
    OP_JMP_INT_CMP_EQ,
    OP_JMP_INT_CMP_NE,
    OP_JMP_INT_CMP_LT,
    OP_JMP_INT_CMP_GT,
    OP_JMP_INT_CMP_LE,
    OP_JMP_INT_CMP_GE,
    OP_JMP_INT_CMP_EQ_IMM8,
    OP_JMP_INT_CMP_NE_IMM8,
    OP_JMP_INT_CMP_LT_IMM8,
    OP_JMP_INT_CMP_GT_IMM8,
    OP_JMP_INT_CMP_LE_IMM8,
    OP_JMP_INT_CMP_GE_IMM8,
};

int insn_has_value(KlrInsn *insn)
{
    for (int i = 0; i < COUNT_OF(no_regs_codes); i++) {
        if (insn->code == no_regs_codes[i]) return 0;
    }
    return 1;
}

/*
 * IR: store_local %var, %val
 *
 * %var is local or parameter of function
 * %val is constant or instruction which has result
 *
 * NOTE: %val which is a local var is not allowed. This is needed to use
 * load_local and then store_local like below:
 *
 * var foo = 100
 * var bar = foo
 *
 * local %foo int
 * store_local %foo, 100
 *
 * local %bar int
 * %0 int = load_local %foo
 * store_local %bar, %0
 */
void klr_build_store(KlrBuilder *bldr, KlrValue *var, KlrValue *val)
{
    if (var->kind != KLR_VALUE_GLOBAL && var->kind != KLR_VALUE_LOCAL &&
        var->kind != KLR_VALUE_PARAM) {
        panic("'set %%x, %%v' requires a local/param/global var.");
    }

    if (val->kind != KLR_VALUE_CONST && val->kind != KLR_VALUE_INSN) {
        panic("'set_local %%x, %%v' requires a reg value.");
    }

    if (!desc_equal(var->desc, val->desc)) {
        panic("'set_local %%x, %%v' requires the same types.");
    }

    KlrInsn *insn = new_insn(OP_IR_STORE, 2, "");
    init_oper(&insn->opers[0], insn, var);
    init_oper(&insn->opers[1], insn, val);
    klr_append_insn(bldr, insn);
}

/*
 * IR: %0 int = load_local %foo
 * %foo is a local or parameter of function
 */
KlrValue *klr_build_load(KlrBuilder *bldr, KlrValue *var)
{
    if (var->kind != KLR_VALUE_GLOBAL && var->kind != KLR_VALUE_LOCAL &&
        var->kind != KLR_VALUE_PARAM) {
        panic("'get %%x, %%v' requires a local/param/global var.");
    }

    KlrInsn *insn = new_insn(OP_IR_LOAD, 1, "");
    init_oper(&insn->opers[0], insn, var);
    insn->desc = var->desc;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_binary(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode op,
                           char *name, const char *op_name)
{
    if (lhs->kind != KLR_VALUE_CONST && lhs->kind != KLR_VALUE_INSN &&
        lhs->kind != KLR_VALUE_PARAM && lhs->kind != KLR_VALUE_LOCAL) {
        panic("'%s %%x, %%y' requires both reg vars/consts", op_name);
    }

    if (rhs->kind != KLR_VALUE_CONST && rhs->kind != KLR_VALUE_INSN &&
        rhs->kind != KLR_VALUE_PARAM && rhs->kind != KLR_VALUE_LOCAL) {
        panic("'%s %%x, %%y' requires both reg vars/consts", op_name);
    }

    KlrInsn *insn = new_insn(op, 2, name);
    init_oper(&insn->opers[0], insn, lhs);
    init_oper(&insn->opers[1], insn, rhs);
    TypeDesc *ty = lhs->desc;
    insn->desc = ty;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_cmp(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode code,
                        char *name)
{
    if (lhs->kind != KLR_VALUE_CONST && lhs->kind != KLR_VALUE_INSN) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    if (rhs->kind != KLR_VALUE_CONST && rhs->kind != KLR_VALUE_INSN) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    KlrInsn *insn = new_insn(code, 2, name);
    init_oper(&insn->opers[0], insn, lhs);
    init_oper(&insn->opers[1], insn, rhs);
    insn->desc = desc_bool();
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_jmp_cond(KlrBuilder *bldr, KlrValue *cond, KlrBasicBlock *_then,
                        KlrBasicBlock *_else)
{
    if (!desc_equal(cond->desc, desc_bool())) {
        panic("'branch %%cond, %%b1, %%b2' requires a bool cond");
    }

    KlrInsn *insn = new_insn(OP_IR_JMP_COND, 4, "");
    init_oper(&insn->opers[0], insn, cond);
    // opers[0] and opers[1] will be remapped ir to virtual machine opcode.
    init_oper(&insn->opers[2], insn, (KlrValue *)_then);
    init_oper(&insn->opers[3], insn, (KlrValue *)_else);
    klr_append_insn(bldr, insn);

    klr_link_edge(bldr->bb, _then);
    klr_link_edge(bldr->bb, _else);
}

void klr_build_jmp(KlrBuilder *bldr, KlrBasicBlock *target)
{
    KlrInsn *insn = new_insn(OP_JMP, 1, "");
    init_oper(&insn->opers[0], insn, (KlrValue *)target);
    klr_append_insn(bldr, insn);

    klr_link_edge(bldr->bb, target);
}

KlrValue *klr_build_call(KlrBuilder *bldr, KlrFunc *fn, KlrValue **args, int nargs,
                         char *name)
{
    KlrInsn *insn = new_insn(OP_CALL, nargs + 1, name);
    init_oper(&insn->opers[0], insn, (KlrValue *)fn);
    for (int j = 0; j < nargs; j++) {
        init_oper(&insn->opers[j + 1], insn, (KlrValue *)args[j]);
    }
    insn->desc = fn->desc;
    klr_append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_ret(KlrBuilder *bldr, KlrValue *ret)
{
    KlrInsn *insn = new_insn(OP_RETURN, 1, "");
    init_oper(&insn->opers[0], insn, ret);
    klr_append_insn(bldr, insn);

    KlrFunc *fn = bldr->bb->func;
    klr_link_edge(bldr->bb, fn->ebb);
}

KlrInsn *klr_new_push(KlrValue *val)
{
    KlrInsn *insn = new_insn(OP_PUSH, 1, "");
    init_oper(&insn->opers[0], insn, val);
    return insn;
}

#ifdef __cplusplus
}
#endif
