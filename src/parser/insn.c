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

static KlrInsn *new_insn(OpCode op, int num_opers, char *name)
{
    KlrInsn *insn = mm_alloc(sizeof(*insn) + sizeof(KlrOper) * num_opers);
    INIT_KLR_VALUE(insn, KLR_VALUE_INSN, NULL, name);
    insn->code = op;
    insn->num_opers = num_opers;
    init_list(&insn->bb_link);
    return insn;
}

static void append_insn(KlrBuilder *bldr, KlrInsn *insn)
{
    KlrBasicBlock *bb = bldr->bb;
    list_add(bldr->it, &insn->bb_link);
    insn->bb = bb;
    bldr->it = &insn->bb_link;
    ++bb->num_insns;
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
    append_insn(bldr, insn);
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
    append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_binary(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, OpCode op,
                           char *name)
{
    if (lhs->kind != KLR_VALUE_CONST && lhs->kind != KLR_VALUE_INSN) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    if (rhs->kind != KLR_VALUE_CONST && rhs->kind != KLR_VALUE_INSN) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    KlrInsn *insn = new_insn(op, 2, name);
    init_oper(&insn->opers[0], insn, lhs);
    init_oper(&insn->opers[1], insn, rhs);
    TypeDesc *ty = lhs->desc;
    insn->desc = ty;
    append_insn(bldr, insn);
    return (KlrValue *)insn;
}

KlrValue *klr_build_cmp(KlrBuilder *bldr, KlrValue *lhs, KlrValue *rhs, char *name)
{
    if (lhs->kind != KLR_VALUE_CONST && lhs->kind != KLR_VALUE_INSN) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    if (rhs->kind != KLR_VALUE_CONST && rhs->kind != KLR_VALUE_INSN) {
        panic("'add %%x, %%y' requires both reg vars/consts");
    }

    KlrInsn *insn = new_insn(OP_BINARY_CMP, 2, name);
    init_oper(&insn->opers[0], insn, lhs);
    init_oper(&insn->opers[1], insn, rhs);
    insn->desc = desc_int8();
    append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_jmp_lt(KlrBuilder *bldr, KlrValue *cond, KlrBasicBlock *_then,
                      KlrBasicBlock *_else)
{
    if (!desc_equal(cond->desc, desc_int8())) {
        panic("'br_lt %%cond, %%b1, %%b2' requires an int8 cond");
    }

    KlrInsn *insn = new_insn(OP_JMP_LTZ, 3, "");
    init_oper(&insn->opers[0], insn, cond);
    init_oper(&insn->opers[1], insn, (KlrValue *)_then);
    init_oper(&insn->opers[2], insn, (KlrValue *)_else);
    append_insn(bldr, insn);

    klr_link_edge(bldr->bb, _then);
    klr_link_edge(bldr->bb, _else);
}

void klr_build_jmp(KlrBuilder *bldr, KlrBasicBlock *target)
{
    KlrInsn *insn = new_insn(OP_JMP, 1, "");
    init_oper(&insn->opers[0], insn, (KlrValue *)target);
    append_insn(bldr, insn);

    klr_link_edge(bldr->bb, target);
}

KlrValue *klr_build_call(KlrBuilder *bldr, KlrFunc *fn, KlrValue **args, char *name)
{
    int i = 0;
    while (args[i]) ++i;

    KlrInsn *insn = new_insn(OP_CALL, i, name);
    for (int j = 0; j < i; j++) {
        init_oper(&insn->opers[j], insn, (KlrValue *)args[j]);
    }
    append_insn(bldr, insn);
    return (KlrValue *)insn;
}

void klr_build_ret(KlrBuilder *bldr, KlrValue *ret)
{
    KlrInsn *insn = new_insn(OP_RETURN, 1, "");
    init_oper(&insn->opers[0], insn, ret);
    append_insn(bldr, insn);

    KlrFunc *fn = bldr->bb->func;
    klr_link_edge(bldr->bb, fn->ebb);
}

#ifdef __cplusplus
}
#endif
