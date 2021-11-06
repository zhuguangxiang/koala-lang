/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"
#include "opcode.h"
#include "util/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static KLVMUse *new_use(KLVMInst *inst, KLVMInterval *interval)
{
    KLVMUse *use = mm_alloc_obj(use);
    use->inst = inst;
    init_list(&use->use_link);
    list_add(&interval->use_list, &use->use_link);
    use->interval = interval;
    return use;
}

static void free_use(KLVMUse *use)
{
    list_remove(&use->use_link);
    use->inst = null;
    use->interval = null;
    mm_free(use);
}

static KLVMInst *alloc_inst(KLVMOpCode op, int num)
{
    KLVMInst *inst = mm_alloc(sizeof(KLVMInst) + sizeof(KLVMOper) * num);
    inst->opcode = op;
    inst->num_ops = num;
    init_list(&inst->link);
    return inst;
}

static void free_inst(KLVMInst *inst)
{
    KLVMOper *oper;
    for (int i = 0; i < inst->num_ops; i++) {
        oper = &inst->operands[i];
        if (oper->kind == KLVM_OPER_REG) {
            free_use(oper->reg);
        }
    }

    mm_free(inst);
}

static inline void append_inst(KLVMBuilder *bldr, KLVMInst *inst)
{
    list_add(bldr->it, &inst->link);
    bldr->it = &inst->link;
    bldr->bb->num_insts++;
}

static void init_const_oper(KLVMOper *oper, KLVMConst *konst)
{
    oper->kind = KLVM_OPER_CONST;
    oper->konst = konst;
}

static void init_var_oper(KLVMOper *oper, KLVMVar *var)
{
    oper->kind = KLVM_OPER_VAR;
    oper->var = var;
}

static void init_func_oper(KLVMOper *oper, KLVMFunc *fn)
{
    oper->kind = KLVM_OPER_FUNC;
    oper->func = fn;
}

static void init_block_oper(KLVMOper *oper, KLVMBasicBlock *block)
{
    oper->kind = KLVM_OPER_BLOCK;
    oper->block = block;
}

static void init_reg_oper(KLVMOper *oper, KLVMInst *inst, KLVMValDef *ref)
{
    oper->kind = KLVM_OPER_REG;
    if (!ref->interval) ref->interval = interval_alloc(ref);
    oper->reg = new_use(inst, ref->interval);
}

static void init_oper(KLVMOper *oper, KLVMInst *inst, KLVMValue *ref)
{
    if (ref->kind == KLVM_VALUE_CONST)
        init_const_oper(oper, (KLVMConst *)ref);
    else
        init_reg_oper(oper, inst, (KLVMValDef *)ref);
}

static int valdef_has_used(KLVMValDef *def)
{
    if (!def->interval) return 0;
    KLVMInterval *interval = def->interval;
    if (list_empty(&interval->use_list)) return 0;

#if !defined(NLOG)
    int count = 0;
    KLVMUse *reg;
    list_foreach(reg, use_link, &interval->use_list, { count++; });
    if (def->label[0]) {
        debug("valdef: %%%s, count: %d", def->label, count);
    } else {
        debug("valdef: %%%d, count: %d", def->tag, count);
    }
#endif

    return 1;
}

void klvm_build_copy(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_COPY, 2);
    // lhs must be instruction
    if (lhs->kind != KLVM_VALUE_REG) panic("expected value definition");

    KLVMValDef *def = (KLVMValDef *)lhs;
    /*
    if (valdef_has_used((KLVMValDef *)lhs)) {
        // FIXME: SSA: new valdef?
        debug("need new value definitons?");
        KLVMFunc *fn = bldr->bb->func;
        KLVMValue *val = klvm_add_local(fn, lhs->type, null);
        def = (KLVMValDef *)val;
    }
    */
    init_reg_oper(&inst->operands[0], inst, def);
    // rhs maybe constant, valdef or var
    init_oper(&inst->operands[1], inst, rhs);
    append_inst(bldr, inst);
}

static int iscmp(KLVMOpCode op)
{
    static KLVMOpCode opcodes[] = {
        KLVM_OP_CMP_EQ, KLVM_OP_CMP_NE, KLVM_OP_CMP_GT,
        KLVM_OP_CMP_GE, KLVM_OP_CMP_LT, KLVM_OP_CMP_LE,
    };

    for (int i = 0; i < COUNT_OF(opcodes); i++) {
        if (op == opcodes[i]) return 1;
    }

    return 0;
}

KLVMValue *klvm_build_binary(KLVMBuilder *bldr, KLVMOpCode op, KLVMValue *lhs,
                             KLVMValue *rhs, char *name)
{
    TypeDesc *ty = iscmp(op) ? desc_from_bool() : lhs->type;
    KLVMInst *inst = alloc_inst(op, 3);

    KLVMValue *def = klvm_add_local(bldr->bb->func, ty, name);
    init_reg_oper(&inst->operands[0], inst, (KLVMValDef *)def);

    // lhs and rhs both maybe constant or valdef.
    init_oper(&inst->operands[1], inst, lhs);
    init_oper(&inst->operands[2], inst, rhs);

    // FIXME:
    // KLVMFunc *fn = bldr->bb->fn;
    // inst->reg = fn->regs++;

    append_inst(bldr, inst);
    return (KLVMValue *)def;
}

KLVMValue *klvm_build_call(KLVMBuilder *bldr, KLVMFunc *fn, KLVMValue *args[],
                           int size, char *name)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_CALL, size + 2);
    TypeDesc *ty = proto_ret(fn->type);
    KLVMValue *def = klvm_add_local(bldr->bb->func, ty, name);
    init_reg_oper(&inst->operands[0], inst, (KLVMValDef *)def);

    init_func_oper(&inst->operands[1], fn);

    KLVMValue *arg;
    for (int i = 1; i < size + 2; i++) {
        arg = args[i];
        init_oper(&inst->operands[i + 1], inst, arg);
    }

    // FIXME:
    // if (type) {
    //     inst->reg = fn->regs++;
    // }

    append_inst(bldr, inst);
    return (KLVMValue *)def;
}

void klvm_build_jmp(KLVMBuilder *bldr, KLVMBasicBlock *dst)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_JMP, 1);
    init_block_oper(&inst->operands[0], dst);
    append_inst(bldr, inst);
    klvm_link_age(bldr->bb, dst);
}

void klvm_build_condjmp(KLVMBuilder *bldr, KLVMValue *cond,
                        KLVMBasicBlock *_then, KLVMBasicBlock *_else)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_COND_JMP, 3);
    init_oper(&inst->operands[0], inst, cond);
    init_block_oper(&inst->operands[1], _then);
    init_block_oper(&inst->operands[2], _else);
    append_inst(bldr, inst);
    klvm_link_age(bldr->bb, _then);
    klvm_link_age(bldr->bb, _else);
}

void klvm_build_ret(KLVMBuilder *bldr, KLVMValue *v)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_RET, 1);
    init_oper(&inst->operands[0], inst, v);
    append_inst(bldr, inst);
    KLVMFunc *fn = bldr->bb->func;
    klvm_link_age(bldr->bb, fn->ebb);
}

void klvm_build_retvoid(KLVMBuilder *bldr)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_RET, 0);
    append_inst(bldr, inst);
    KLVMFunc *fn = bldr->bb->func;
    klvm_link_age(bldr->bb, fn->ebb);
}

#ifdef __cplusplus
}
#endif
