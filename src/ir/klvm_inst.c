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

KLVMValue *KLVMBuildBinary(KLVMBuilder *bldr, KLVMBinaryOp op, KLVMValue *lhs,
    KLVMValue *rhs, const char *name)
{
    KLVMBinaryInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, lhs->type, KLVM_INST_BINARY, name);
    inst->bop = op;
    inst->lhs = lhs;
    inst->rhs = rhs;
    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
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

void KLVMBuildJmp(KLVMBuilder *bldr, KLVMBasicBlock *dest)
{
    KLVMJmpInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, KLVM_INST_JMP, NULL);
    inst->dest = dest;
    _inst_append(bldr, (KLVMInst *)inst);
}

void KLVMBuildCondJmp(KLVMBuilder *bldr, KLVMValue *cond, KLVMBasicBlock *_then,
    KLVMBasicBlock *_else)
{
    KLVMCondJmpInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, KLVM_INST_JMP_COND, NULL);
    inst->cond = cond;
    inst->_then = _then;
    inst->_else = _else;
    _inst_append(bldr, (KLVMInst *)inst);
}

void KLVMBuildRet(KLVMBuilder *bldr, KLVMValue *v)
{
    KLVMRetInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, v->type, KLVM_INST_RET, NULL);
    inst->ret = v;
    _inst_append(bldr, (KLVMInst *)inst);
}

void KLVMBuildRetVoid(KLVMBuilder *bldr)
{
    KLVMInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, NULL, KLVM_INST_RET_VOID, NULL);
    _inst_append(bldr, (KLVMInst *)inst);
}

#ifdef __cplusplus
}
#endif
