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

    KLVMVar *var;

    if (lhs->kind == VALUE_VAR) {
        var = (KLVMVar *)lhs;
        inst->lhs = KLVMReference(lhs, var->name);
    }
    else {
        inst->lhs = lhs;
    }

    if (rhs->kind == VALUE_VAR) {
        var = (KLVMVar *)rhs;
        inst->rhs = KLVMReference(rhs, var->name);
    }
    else {
        inst->rhs = rhs;
    }

    return (KLVMValue *)inst;
}

static inline void _inst_append(KLVMBuilder *bldr, KLVMInst *inst)
{
    list_add(bldr->rover, &inst->node);
    bldr->rover = &inst->node;
    bldr->bb->num_insts++;
}

void KLVMBuildStore(KLVMBuilder *bldr, KLVMValue *rhs, KLVMValue *lhs)
{
    if (KLVMTypeCheck(lhs->type, rhs->type)) {
        printf("error: type not matched\n");
    }

    KLVMStoreInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, lhs->type, KLVM_INST_STORE, NULL);
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

KLVMValue *KLVMBuildRet(KLVMBuilder *bldr, KLVMValue *v)
{
    KLVMReturnInst *inst = mm_alloc(sizeof(*inst));
    INIT_INST_HEAD(inst, v->type, KLVM_INST_RET, v->name);
    if (v->kind == VALUE_CONST)
        inst->ret = v;
    else
        inst->ret = KLVMReference(v, v->name);
    _inst_append(bldr, (KLVMInst *)inst);
    return (KLVMValue *)inst;
}

#ifdef __cplusplus
}
#endif
