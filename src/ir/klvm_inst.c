/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

KLVMValue *KLVMCreateBinaryInst(
    KLVMInstKind kind, KLVMValue *lhs, KLVMValue *rhs, const char *name)
{
    KLVMBinaryInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, lhs->type, kind, name);
    inst->lhs = lhs;
    inst->rhs = rhs;
    return (KLVMValue *)inst;
}

static inline void _inst_append(KLVMBuilder *bldr, KLVMInst *inst)
{
    list_add(bldr->rover, &inst->node);
    bldr->rover = &inst->node;
    bldr->bb->num_insts++;
}

void KLVMBuildCopy(KLVMBuilder *bldr, KLVMValue *rhs, KLVMValue *lhs)
{
    if (KLVMTypeCheck(lhs->type, rhs->type)) {
        printf("error: type not matched\n");
    }

    KLVMCopyInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, lhs->type, KLVM_INST_COPY, NULL);
    inst->lhs = lhs;
    inst->rhs = rhs;

    _inst_append(bldr, (KLVMInst *)inst);
}

KLVMValue *KLVMBuildAdd(
    KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs, const char *name)
{
    KLVMValue *inst = KLVMCreateBinaryInst(KLVM_INST_ADD, lhs, rhs, name);
    _inst_append(bldr, (KLVMInst *)inst);
    return inst;
}

KLVMValue *KLVMBuildSub(
    KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs, const char *name)
{
    KLVMValue *inst = KLVMCreateBinaryInst(KLVM_INST_SUB, lhs, rhs, name);
    _inst_append(bldr, (KLVMInst *)inst);
    return inst;
}

KLVMValue *KLVMBuildCall(
    KLVMBuilder *bldr, KLVMValue *fn, KLVMValue **args, const char *name)
{
    KLVMCallInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, KLVM_INST_CALL, name);
    inst->fn = fn;
    vector_init(&inst->args, sizeof(void *));

    KLVMValue **arg = args;
    while (*arg) {
        vector_push_back(&inst->args, arg);
        arg++;
    }

    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
}

KLVMValue *KLVMBuildJump(KLVMBuilder *bldr, KLVMBasicBlock *dest)
{
    KLVMJumpInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, KLVM_INST_JMP, NULL);
    inst->dest = dest;
    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
}

KLVMValue *KLVMBuildCondJump(KLVMBuilder *bldr, KLVMInstKind op, KLVMValue *lhs,
    KLVMValue *rhs, KLVMBasicBlock *_then, KLVMBasicBlock *_else)
{
    KLVMCondJumpInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, op, NULL);
    inst->lhs = lhs;
    inst->rhs = rhs;
    inst->_then = _then;
    inst->_else = _else;
    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
}

KLVMValue *KLVMBuildRet(KLVMBuilder *bldr, KLVMValue *v)
{
    KLVMRetInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, v->type, KLVM_INST_RET, v->name);
    inst->ret = v;
    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
}

KLVMValue *KLVMBuildRetVoid(KLVMBuilder *bldr)
{
    KLVMInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, KLVM_INST_RET_VOID, NULL);
    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
}

#ifdef __cplusplus
}
#endif
