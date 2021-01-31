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

/*
    https://cs.lmu.edu/~ray/notes/ir/
 */

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

    printf("%s %s\n", var->name, KLVMTypeString(var->type));
}

static void inst_show_value(KLVMValue *val)
{
    KLVMValueKind kind = val->kind;
    switch (kind) {
        case VALUE_CONST: {
            KLVMConstShow((KLVMConst *)val);
            break;
        }
        case VALUE_VAR: {
            KLVMVar *var = (KLVMVar *)val;
            if (var->flags & KLVM_FLAGS_GLOBAL)
                printf("@%s", var->name);
            else {
                if (var->name)
                    printf("%%%s", var->name);
                else
                    printf("%%%d", var->tag);
            }
            break;
        }
        case VALUE_INST: {
            KLVMInst *inst = (KLVMInst *)val;
            printf("%%%d", inst->tag);
            break;
        }
        case VALUE_FUNC: {
            KLVMFunc *fn = (KLVMFunc *)val;
            printf("@%s", fn->name);
            break;
        }
        default:
            printf("error:invalid value\n");
            abort();
            break;
    }
}

static void type_show(KLVMType *ty)
{
    printf("%s", KLVMTypeString(ty));
}

static void inst_binary_show(KLVMBinaryInst *inst)
{
    switch (inst->bop) {
        case KLVM_BINARY_ADD:
            printf("%%%d = add ", inst->tag);
            break;
        case KLVM_BINARY_SUB:
            printf("%%%d = sub ", inst->tag);
            break;
        case KLVM_BINARY_CMP_LE:
            printf("%%%d = cmp le ", inst->tag);
            break;
        default:
            printf("error:invalid binary instruction\n");
            abort();
            break;
    }

    inst_show_value(((KLVMBinaryInst *)inst)->lhs);
    printf(", ");
    inst_show_value(((KLVMBinaryInst *)inst)->rhs);
}

static void KLVMInstShow(KLVMInst *inst)
{
    switch (inst->op) {
        case KLVM_INST_COPY:
            inst_show_value(((KLVMCopyInst *)inst)->lhs);
            printf(" = ");
            inst_show_value(((KLVMCopyInst *)inst)->rhs);
            break;
        case KLVM_INST_BINARY:
            inst_binary_show((KLVMBinaryInst *)inst);
            break;
        case KLVM_INST_CALL: {
            KLVMCallInst *call = (KLVMCallInst *)inst;
            printf("%%%d = call ", call->tag);
            inst_show_value(call->fn);
            printf("(");
            KLVMValue **val;
            vector_foreach(val, &call->args)
            {
                if (i != 0) printf(", ");
                inst_show_value(*val);
            }
            printf(") ");
            type_show(KLVMProtoTypeReturn(((KLVMFunc *)call->fn)->type));
            break;
        }
        case KLVM_INST_RET:
            printf("ret ");
            inst_show_value(((KLVMRetInst *)inst)->ret);
            break;
        case KLVM_INST_JMP_COND:
            printf("br ");
            inst_show_value(((KLVMCondJmpInst *)inst)->cond);
            printf(", label %%%s", ((KLVMCondJmpInst *)inst)->_then->label);
            printf(", label %%%s", ((KLVMCondJmpInst *)inst)->_else->label);
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
        default:
            break;
    }
}

static void KLVMBasicBlockShow(KLVMBasicBlock *bb, int indent)
{
    if (bb->label)
        printf("%%%s:\n", bb->label);
    else
        printf("%%bb%d:\n", bb->tag);
    KLVMInst *inst;
    ListNode *n;
    list_foreach(&bb->insts, n)
    {
        inst = container_of(n, KLVMInst, node);
        if (indent) printf("  ");
        KLVMInstShow(inst);
        printf("\n");
    }
}

static void KLVMFuncBodyShow(KLVMFunc *fn, int indent)
{
    KLVMVar *var;
    int num_locals = vector_size(&fn->locals);
    for (int i = fn->num_params; i < num_locals; i++) {
        vector_get(&fn->locals, i, &var);
        if (indent) printf("  ");
        KLVMVarShow(var);
    }

    KLVMBasicBlock *bb;
    ListNode *n;
    list_foreach(&fn->bblist, n)
    {
        bb = (KLVMBasicBlock *)n;
        KLVMBasicBlockShow(bb, indent);
    }
}

static void KLVMFuncShow(KLVMFunc *fn)
{
    char buf[4096];
    buf[0] = '\0';

    strcat(buf, "(");
    KLVMVar *para;
    for (int i = 0; i < fn->num_params; i++) {
        vector_get(&fn->locals, i, &para);
        if (i != 0) strcat(buf, ", ");
        if (para->name)
            strcat(buf, para->name);
        else {
            int l = strlen(buf);
            sprintf(buf + l, "%%%d", para->tag);
        }
        strcat(buf, " ");
        strcat(buf, KLVMTypeString(para->type));
    }
    strcat(buf, ")");

    KLVMType *ret = KLVMProtoTypeReturn(fn->type);
    if (ret) {
        strcat(buf, " ");
        strcat(buf, KLVMTypeString(para->type));
    }

    printf("func %s%s {\n", fn->name, buf);

    KLVMFuncBodyShow(fn, 2);

    printf("}\n");
}

void KLVMDumpModule(KLVMModule *m)
{
    printf("\n__name__ = \"%s\";\n\n", m->name);

    KLVMVar **var;
    vector_foreach(var, &m->vars)
    {
        KLVMVarShow(*var);
    }

    printf("\n");

    KLVMFuncShow((KLVMFunc *)KLVMModuleFunc(m));

    printf("\n");

    KLVMFunc **fn;
    vector_foreach(fn, &m->funcs)
    {
        KLVMFuncShow(*fn);
        printf("\n");
    }
}

#ifdef __cplusplus
}
#endif
