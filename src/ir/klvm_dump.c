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

static void KLVMValueShow(KLVMValue *val);

static void KLVMConstShow(KLVMConst *kval)
{
    if (kval->type->kind == KLVM_TYPE_INT32) { printf("%d", kval->i32val); }
}

static void KLVMVarShow(KLVMVar *var)
{
    if (var->flags & KLVM_FLAGS_CONST)
        printf("const ");
    else
        printf("var ");

    printf("%s %s;\n", var->name, KLVMTypeString(var->type));
}

static void KLVMRefShow(KLVMRef *ref)
{
    printf("%s", ref->name);
}

static void KLVMStoreInstLeftShow(KLVMValue *val)
{
    if (val->kind == VALUE_VAR) { printf("%s", val->name); }
}

static void KLVMStoreInstRightShow(KLVMValue *val)
{
    if (val->kind == VALUE_INST) { printf("%s", val->name); }
    else if (val->kind == VALUE_CONST) {
        KLVMConstShow((KLVMConst *)val);
    }
}

static void KLVMInstShow(KLVMInst *inst)
{
    switch (inst->op) {
        case KLVM_INST_LOAD:
            break;
        case KLVM_INST_STORE:
            KLVMStoreInstLeftShow(((KLVMStoreInst *)inst)->lhs);
            printf(" = ");
            KLVMStoreInstRightShow(((KLVMStoreInst *)inst)->rhs);
            break;
        case KLVM_INST_ADD:
            printf("  %s := ", inst->name);
            KLVMValueShow(((KLVMBinaryInst *)inst)->lhs);
            printf(" + ");
            KLVMValueShow(((KLVMBinaryInst *)inst)->rhs);
            break;
        case KLVM_INST_SUB:
            printf("  %s := ", inst->name);
            KLVMValueShow(((KLVMBinaryInst *)inst)->lhs);
            printf(" - ");
            KLVMValueShow(((KLVMBinaryInst *)inst)->rhs);
            break;
        case KLVM_INST_RET:
            printf("  return ");
            KLVMValueShow(((KLVMReturnInst *)inst)->ret);
            break;
        default:
            break;
    }
}

static void KLVMValueShow(KLVMValue *val)
{
    KLVMValueKind kind = val->kind;
    switch (kind) {
        case VALUE_CONST: {
            KLVMConstShow((KLVMConst *)val);
            break;
        }
        case VALUE_INST: {
            KLVMInstShow((KLVMInst *)val);
            break;
        }
        case VALUE_VAR: {
            KLVMVarShow((KLVMVar *)val);
            break;
        }
        case VALUE_REF: {
            KLVMRefShow((KLVMRef *)val);
            break;
        }
        default:
            break;
    }
}

static void KLVMBasicBlockShow(KLVMBasicBlock *bb)
{
    KLVMInst *inst;
    ListNode *n;
    list_foreach(&bb->insts, n)
    {
        inst = container_of(n, KLVMInst, node);
        KLVMInstShow(inst);
        printf("\n");
    }
}

static void KLVMFuncBodyShow(KLVMFunction *fn)
{
    KLVMVar *var;
    int num_locals = vector_size(&fn->locals);
    for (int i = fn->num_params; i < num_locals; i++) {
        vector_get(&fn->locals, i, &var);
        KLVMVarShow(var);
    }

    KLVMBasicBlock *bb;
    ListNode *n;
    list_foreach(&fn->bblist, n)
    {
        bb = (KLVMBasicBlock *)n;
        KLVMBasicBlockShow(bb);
    }
}

static void KLVMFuncShow(KLVMFunction *fn)
{
    char buf[4096];
    buf[0] = '\0';

    strcat(buf, "(");
    KLVMVar *para;
    for (int i = 0; i < fn->num_params; i++) {
        vector_get(&fn->locals, i, &para);
        if (i != 0) strcat(buf, ", ");
        strcat(buf, para->name);
        strcat(buf, " ");
        strcat(buf, KLVMTypeString(para->type));
    }
    strcat(buf, ")");

    KLVMType *ret = KLVMProtoTypeReturn(fn->type);
    if (ret) {
        strcat(buf, " ");
        strcat(buf, KLVMTypeString(para->type));
    }

    printf("func %s %s {\n", fn->name, buf);

    KLVMFuncBodyShow(fn);

    printf("}\n");
}

void KLVMDumpModule(KLVMModule *m)
{
    printf("\n__name__ := \"%s\";\n\n", m->name);

    KLVMVar **var;
    vector_foreach(var, &m->vars)
    {
        KLVMVarShow(*var);
    }

    printf("\n");

    KLVMFuncBodyShow((KLVMFunction *)m->fn);

    printf("\n");

    KLVMFunction **fn;
    vector_foreach(fn, &m->funcs)
    {
        KLVMFuncShow(*fn);
    }

    printf("\n");
}

#ifdef __cplusplus
}
#endif
