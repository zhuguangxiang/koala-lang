/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"
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

static KLVMInst *alloc_inst(KlOpCode op, int num, char *name)
{
    KLVMInst *inst = mm_alloc(sizeof(*inst) + sizeof(KLVMOper) * num);
    inst->kind = KLVM_VALUE_INST;
    init_list(&inst->use_list);
    inst->name = name ? name : "";
    inst->opcode = op;
    inst->num_ops = num;
    inst->vreg = -1;
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
    ++bldr->bb->num_insts;
    ++bldr->bb->func->num_insts;
}

static void init_operand(KLVMOper *oper, KLVMInst *inst, KLVMValue *ref)
{
    switch (ref->kind) {
        case KLVM_VALUE_CONST:
            oper->kind = KLVM_OPER_CONST;
            oper->konst = (KLVMConst *)ref;
            break;
        case KLVM_VALUE_VAR:
            oper->kind = KLVM_OPER_VAR;
            oper->use = new_use(inst, ref);
            break;
        case KLVM_VALUE_FUNC:
            oper->kind = KLVM_OPER_FUNC;
            oper->use = new_use(inst, ref);
            break;
        case KLVM_VALUE_BLOCK:
            oper->kind = KLVM_OPER_BLOCK;
            oper->use = new_use(inst, ref);
            break;
        case KLVM_VALUE_ARG:
        case KLVM_VALUE_LOCAL:
        case KLVM_VALUE_INST:
            oper->kind = KLVM_OPER_REG;
            oper->use = new_use(inst, ref);
            break;
        default:
            panic("invalid value");
            break;
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
    local->vreg = bb->func->vregs++;
    return (KLVMValue *)local;
}

void klvm_get_uses(KLVMInst *inst, Vector *uses)
{
    KLVMOper *oper;

    if (inst->opcode == OP_MOVE) {
        oper = &inst->operands[1];
        if (oper->kind == KLVM_OPER_REG) {
            vector_push_back(uses, &oper->use->parent);
        }
        return;
    }

    for (int i = 0; i < inst->num_ops; i++) {
        oper = &inst->operands[i];
        if (oper->kind == KLVM_OPER_REG) {
            vector_push_back(uses, &oper->use->parent);
        }
    }
}

KLVMValue *klvm_get_def(KLVMInst *inst)
{
    KLVMOper *oper;

    if (inst->opcode >= OP_MOVE && inst->opcode <= OP_LOAD_CONST) {
        oper = &inst->operands[0];
        if (oper->kind == KLVM_OPER_REG) {
            return oper->use->parent;
        } else {
            return null;
        }
    }

    if (inst->type) {
        return (KLVMValue *)inst;
    } else {
        return null;
    }
}

void klvm_build_copy(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    if (lhs->kind != KLVM_VALUE_VAR && lhs->kind != KLVM_VALUE_ARG &&
        lhs->kind != KLVM_VALUE_LOCAL) {
        panic("expected value user");
    }

    KlOpCode opcode = OP_MOVE;
    if (rhs->kind == KLVM_VALUE_CONST) {
        static KlOpCode map[] = {
            0,
            OP_CONST_I8,
            OP_CONST_I16,
            OP_CONST_I32,
            OP_LOAD_CONST,
            OP_CONST_F32,
            OP_LOAD_CONST,
            OP_CONST_BOOL,
            OP_CONST_CHAR,
        };
        KLVMConst *konst = (KLVMConst *)rhs;
        TypeDesc *type = rhs->type;
        if (type->kind >= TYPE_I8_KIND && type->kind <= TYPE_CHAR_KIND) {
            opcode = map[type->kind];
        }
    }

    KLVMInst *inst = alloc_inst(opcode, 2, null);
    init_operand(&inst->operands[0], inst, lhs);
    init_operand(&inst->operands[1], inst, rhs);
    append_inst(bldr, inst);
}

static int iscmp(KlOpCode op)
{
    static KlOpCode opcodes[] = {
        OP_CMP_EQ, OP_CMP_NE, OP_CMP_GT, OP_CMP_GE, OP_CMP_LT, OP_CMP_LE,
    };

    for (int i = 0; i < COUNT_OF(opcodes); i++) {
        if (op == opcodes[i]) return 1;
    }

    return 0;
}

static KLVMValue *klvm_build_binary(KLVMBuilder *bldr, KlOpCode op,
                                    KLVMValue *lhs, KLVMValue *rhs)
{
    TypeDesc *ty = iscmp(op) ? desc_from_bool() : lhs->type;
    KLVMInst *inst = alloc_inst(op, 2, null);
    inst->type = ty;

    // lhs and rhs both maybe constant or user.
    init_operand(&inst->operands[0], inst, lhs);
    init_operand(&inst->operands[1], inst, rhs);

    append_inst(bldr, inst);
    inst->vreg = bldr->bb->func->vregs++;
    return (KLVMValue *)inst;
}

KLVMValue *klvm_build_add(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_ADD, lhs, rhs);
}

KLVMValue *klvm_build_sub(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_SUB, lhs, rhs);
}

KLVMValue *klvm_build_mul(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_MUL, lhs, rhs);
}

KLVMValue *klvm_build_div(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_DIV, lhs, rhs);
}

KLVMValue *klvm_build_mod(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_MOD, lhs, rhs);
}

KLVMValue *klvm_build_cmple(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_CMP_LE, lhs, rhs);
}

KLVMValue *klvm_build_cmplt(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_CMP_LT, lhs, rhs);
}

KLVMValue *klvm_build_cmpge(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_CMP_GE, lhs, rhs);
}

KLVMValue *klvm_build_cmpgt(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_CMP_GT, lhs, rhs);
}

KLVMValue *klvm_build_cmpeq(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_CMP_EQ, lhs, rhs);
}

KLVMValue *klvm_build_cmpne(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs)
{
    return klvm_build_binary(bldr, OP_CMP_NE, lhs, rhs);
}

KLVMValue *klvm_build_call(KLVMBuilder *bldr, KLVMFunc *fn, KLVMValue *args[],
                           int size)
{
    TypeDesc *ty = proto_ret(fn->type);
    KLVMInst *inst = alloc_inst(OP_CALL, size + 1, null);
    inst->type = ty;

    init_operand(&inst->operands[0], inst, (KLVMValue *)fn);

    KLVMValue *arg;
    for (int i = 0; i < size; i++) {
        arg = args[i];
        init_operand(&inst->operands[i + 1], inst, arg);
    }

    append_inst(bldr, inst);
    if (ty) inst->vreg = bldr->bb->func->vregs++;
    return ty ? (KLVMValue *)inst : null;
}

void klvm_build_jmp(KLVMBuilder *bldr, KLVMBasicBlock *dst)
{
    KLVMInst *inst = alloc_inst(OP_JMP, 1, null);
    init_operand(&inst->operands[0], inst, (KLVMValue *)dst);
    append_inst(bldr, inst);
    klvm_link_age(bldr->bb, dst);
}

void klvm_build_condjmp(KLVMBuilder *bldr, KLVMValue *cond,
                        KLVMBasicBlock *_true, KLVMBasicBlock *_false)
{
    KlOpCode opcode = 0;
    int opnum = 3;
    if (cond->kind == KLVM_VALUE_INST) {
        KLVMInst *inst = (KLVMInst *)cond;
        if (inst->opcode >= OP_CMP_EQ && inst->opcode <= OP_CMP_CONST_LE) {
            opcode = OP_JMP_EQ + (inst->opcode - OP_CMP_EQ);
            opnum = 4;
        }
    }
    KLVMInst *inst = alloc_inst(opcode, opnum, null);
    if (opnum == 3) {
        assert(0);
        init_operand(&inst->operands[0], inst, cond);
        init_operand(&inst->operands[1], inst, (KLVMValue *)_true);
        init_operand(&inst->operands[2], inst, (KLVMValue *)_false);
    } else {
        KLVMInst *cond_inst = (KLVMInst *)cond;
        KLVMOper *oper = &cond_inst->operands[0];
        if (oper->kind == KLVM_OPER_CONST) {
            init_operand(&inst->operands[0], inst, (KLVMValue *)oper->konst);
        } else {
            init_operand(&inst->operands[0], inst, oper->use->parent);
        }
        oper = &cond_inst->operands[1];
        if (oper->kind == KLVM_OPER_CONST) {
            init_operand(&inst->operands[1], inst, (KLVMValue *)oper->konst);
        } else {
            init_operand(&inst->operands[1], inst, oper->use->parent);
        }
        init_operand(&inst->operands[2], inst, (KLVMValue *)_true);
        init_operand(&inst->operands[3], inst, (KLVMValue *)_false);
    }

    append_inst(bldr, inst);
    klvm_link_age(bldr->bb, _true);
    klvm_link_age(bldr->bb, _false);
}

void klvm_build_ret(KLVMBuilder *bldr, KLVMValue *v)
{
    KLVMInst *inst = alloc_inst(OP_RET, 1, null);
    init_operand(&inst->operands[0], inst, v);
    append_inst(bldr, inst);
    KLVMFunc *fn = bldr->bb->func;
    klvm_link_age(bldr->bb, fn->ebb);
}

void klvm_build_ret_void(KLVMBuilder *bldr)
{
    KLVMInst *inst = alloc_inst(OP_RET, 0, null);
    append_inst(bldr, inst);
    KLVMFunc *fn = bldr->bb->func;
    klvm_link_age(bldr->bb, fn->ebb);
}

#ifdef __cplusplus
}
#endif
