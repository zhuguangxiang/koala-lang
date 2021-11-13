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

static KLVMUse *new_use(KLVMInst *inst, KLVMValue *val)
{
    KLVMUse *use = mm_alloc_obj(use);
    use->inst = inst;
    init_list(&use->use_link);
    list_add(&val->use_list, &use->use_link);
    use->parent = val;
    return use;
}

/*
static void free_use(KLVMUse *use)
{
    list_remove(&use->use_link);
    use->inst = null;
    use->parent = null;
    mm_free(use);
}
*/

static KLVMInst *alloc_inst(KLVMOpCode op, int num, char *name)
{
    KLVMInst *inst = mm_alloc(sizeof(*inst) + sizeof(KLVMOper) * num);
    inst->kind = KLVM_VALUE_INST;
    init_list(&inst->use_list);
    inst->name = name ? name : "";
    inst->opcode = op;
    inst->num_ops = num;
    init_list(&inst->bb_link);
    return inst;
}

/*
void klvm_free_const(KLVMConst *konst);

static void free_inst(KLVMInst *inst)
{
    KLVMOper *oper;
    for (int i = 0; i < inst->num_ops; i++) {
        oper = &inst->operands[i];
        if (oper->kind == KLVM_OPER_USE) {
            free_use(oper->use);
        } else {
            klvm_free_const(oper->konst);
        }
    }

    mm_free(inst);
}
*/

static void append_inst(KLVMBuilder *bldr, KLVMInst *inst)
{
    list_add(bldr->it, &inst->bb_link);
    inst->bb = bldr->bb;
    bldr->it = &inst->bb_link;
}

static void init_operand(KLVMOper *oper, KLVMInst *inst, KLVMValue *ref)
{
    if (ref->kind == KLVM_VALUE_CONST) {
        oper->kind = KLVM_OPER_CONST;
        oper->konst = (KLVMConst *)ref;
    } else {
        oper->kind = KLVM_OPER_USE;
        oper->use = new_use(inst, ref);
    }
}

static KLVMLocal *__new_local(TypeDesc *ty, char *name)
{
    KLVMLocal *local = mm_alloc_obj(local);
    local->kind = KLVM_VALUE_LOCAL;
    init_list(&local->use_list);
    local->name = name ? name : "";
    local->type = ty;
    init_list(&local->bb_link);
    return local;
}

KLVMValue *klvm_build_local(KLVMBuilder *bldr, TypeDesc *ty, char *name)
{
    KLVMBasicBlock *bb = bldr->bb;
    KLVMLocal *local = __new_local(ty, name);
    list_push_back(&bb->local_list, &local->bb_link);
    local->bb = bb;
    return (KLVMValue *)local;
}

void klvm_build_copy(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_COPY, 2, null);

    if (lhs->kind != KLVM_VALUE_VAR && lhs->kind != KLVM_VALUE_ARG &&
        lhs->kind != KLVM_VALUE_LOCAL) {
        panic("expected value user");
    }

    init_operand(&inst->operands[0], inst, lhs);
    init_operand(&inst->operands[1], inst, rhs);
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

static KLVMValue *klvm_build_binary(KLVMBuilder *bldr, KLVMOpCode op,
                                    KLVMValue *lhs, KLVMValue *rhs)
{
    TypeDesc *ty = iscmp(op) ? desc_from_bool() : lhs->type;
    KLVMInst *inst = alloc_inst(op, 2, null);
    inst->type = ty;

    // lhs and rhs both maybe constant or user.
    init_operand(&inst->operands[0], inst, lhs);
    init_operand(&inst->operands[1], inst, rhs);

    append_inst(bldr, inst);
    return (KLVMValue *)inst;
}

KLVMValue *klvm_build_add(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_ADD, lhs, rhs);
}

KLVMValue *klvm_build_sub(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_SUB, lhs, rhs);
}

KLVMValue *klvm_build_mul(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_MUL, lhs, rhs);
}

KLVMValue *klvm_build_div(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_DIV, lhs, rhs);
}

KLVMValue *klvm_build_mod(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_MOD, lhs, rhs);
}

KLVMValue *klvm_build_cmple(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_CMP_LE, lhs, rhs);
}

KLVMValue *klvm_build_cmplt(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_CMP_LT, lhs, rhs);
}

KLVMValue *klvm_build_cmpge(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_CMP_GE, lhs, rhs);
}

KLVMValue *klvm_build_cmpgt(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_CMP_GT, lhs, rhs);
}

KLVMValue *klvm_build_cmpeq(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_CMP_EQ, lhs, rhs);
}

KLVMValue *klvm_build_cmpne(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, KLVM_OP_CMP_NE, lhs, rhs);
}

KLVMValue *klvm_build_call(KLVMBuilder *bldr, KLVMFunc *fn, KLVMValue *args[],
                           int size)
{
    TypeDesc *ty = proto_ret(fn->type);
    KLVMInst *inst = alloc_inst(KLVM_OP_CALL, size + 1, null);
    inst->type = ty;

    init_operand(&inst->operands[0], inst, (KLVMValue *)fn);

    KLVMValue *arg;
    for (int i = 0; i < size; i++) {
        arg = args[i];
        init_operand(&inst->operands[i + 1], inst, arg);
    }

    append_inst(bldr, inst);
    return ty ? (KLVMValue *)inst : null;
}

void klvm_build_jmp(KLVMBuilder *bldr, KLVMBasicBlock *dst)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_JMP, 1, null);
    init_operand(&inst->operands[0], inst, (KLVMValue *)dst);
    append_inst(bldr, inst);
    klvm_link_age(bldr->bb, dst);
}

void klvm_build_condjmp(KLVMBuilder *bldr, KLVMValue *cond,
                        KLVMBasicBlock *_then, KLVMBasicBlock *_else)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_COND_JMP, 3, null);
    init_operand(&inst->operands[0], inst, cond);
    init_operand(&inst->operands[1], inst, (KLVMValue *)_then);
    init_operand(&inst->operands[2], inst, (KLVMValue *)_else);
    append_inst(bldr, inst);
    klvm_link_age(bldr->bb, _then);
    klvm_link_age(bldr->bb, _else);
}

void klvm_build_ret(KLVMBuilder *bldr, KLVMValue *v)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_RET, 1, null);
    init_operand(&inst->operands[0], inst, v);
    append_inst(bldr, inst);
    KLVMFunc *fn = bldr->bb->func;
    klvm_link_age(bldr->bb, fn->ebb);
}

void klvm_build_retvoid(KLVMBuilder *bldr)
{
    KLVMInst *inst = alloc_inst(KLVM_OP_RET, 0, null);
    append_inst(bldr, inst);
    KLVMFunc *fn = bldr->bb->func;
    klvm_link_age(bldr->bb, fn->ebb);
}

#ifdef __cplusplus
}
#endif
