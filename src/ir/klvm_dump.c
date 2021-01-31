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

    printf("%s %s\n", var->name, KLVMTypeString(var->type));
}

static void inst_copy_left(KLVMValue *val)
{
    if (val->kind == VALUE_VAR) { printf("%s", val->name); }
}

static void inst_copy_right(KLVMValue *val)
{
    if (val->kind == VALUE_INST) { printf("%%%s", val->name); }
    else if (val->kind == VALUE_CONST) {
        KLVMConstShow((KLVMConst *)val);
    }
}

static void inst_show_left(KLVMValue *lhs)
{
    KLVMValueKind kind = lhs->kind;
    switch (kind) {
        case VALUE_CONST: {
            KLVMConstShow((KLVMConst *)lhs);
            break;
        }
        case VALUE_VAR: {
            printf("%s", lhs->name);
            break;
        }
        case VALUE_INST: {
            printf("%%%s", lhs->name);
            break;
        }
        default:
            printf("error:invalid lhs value\n");
            abort();
            break;
    }
}

static void inst_show_right(KLVMValue *rhs)
{
    KLVMValueKind kind = rhs->kind;
    switch (kind) {
        case VALUE_CONST: {
            KLVMConstShow((KLVMConst *)rhs);
            break;
        }
        case VALUE_VAR: {
            printf("%s", rhs->name);
            break;
        }
        case VALUE_INST: {
            printf("%%%s", rhs->name);
            break;
        }
        default:
            printf("error:invalid rhs value\n");
            abort();
            break;
    }
}

static void inst_ret_show(KLVMValue *ret)
{
    KLVMValueKind kind = ret->kind;
    switch (kind) {
        case VALUE_CONST: {
            KLVMConstShow((KLVMConst *)ret);
            break;
        }
        case VALUE_VAR: {
            printf("%s", ret->name);
            break;
        }
        case VALUE_INST: {
            printf("%%%s", ret->name);
            break;
        }
        default:
            printf("error:invalid ret value\n");
            abort();
            break;
    }
}

static void KLVMBasicBlockShow(KLVMBasicBlock *bb, int indent);

static void KLVMInstShow(KLVMInst *inst)
{
    switch (inst->op) {
        case KLVM_INST_COPY:
            inst_copy_left(((KLVMCopyInst *)inst)->lhs);
            printf(" = ");
            inst_copy_right(((KLVMCopyInst *)inst)->rhs);
            break;
        case KLVM_INST_ADD:
            printf("%%%s = ", inst->name);
            inst_show_left(((KLVMBinaryInst *)inst)->lhs);
            printf(" + ");
            inst_show_right(((KLVMBinaryInst *)inst)->rhs);
            break;
        case KLVM_INST_SUB:
            printf("%%%s = ", inst->name);
            inst_show_left(((KLVMBinaryInst *)inst)->lhs);
            printf(" - ");
            inst_show_right(((KLVMBinaryInst *)inst)->rhs);
            break;
        case KLVM_INST_CALL: {
            printf("%%%s = call ", inst->name);
            printf("%s", ((KLVMCallInst *)inst)->fn->name);
            int num_args = vector_size(&((KLVMCallInst *)inst)->args);
            printf(", %d", num_args);
            break;
        }
        case KLVM_INST_RET:
            printf("ret ");
            inst_ret_show(((KLVMRetInst *)inst)->ret);
            break;
        case KLVM_INST_JMP_LE:
            printf("ble ");
            inst_show_left(((KLVMCondJumpInst *)inst)->lhs);
            printf(", ");
            inst_show_right(((KLVMCondJumpInst *)inst)->rhs);
            printf(", label %%%s", ((KLVMCondJumpInst *)inst)->_then->name);
            printf(", label %%%s", ((KLVMCondJumpInst *)inst)->_else->name);
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
    printf("%%%s:\n", bb->name);
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
        printf("\n");
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

    KLVMFuncBodyShow((KLVMFunc *)KLVMModuleFunc(m), 0);

    printf("\n");

    KLVMFunc **fn;
    vector_foreach(fn, &m->funcs)
    {
        KLVMFuncShow(*fn);
        printf("\n");
    }

    printf("\n");
}

#ifdef __cplusplus
}
#endif
