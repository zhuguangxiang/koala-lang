/*----------------------------------------------------------------------------*\
|* This file is part of the koala-lang project, under the MIT License.        *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

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

static inline int val_tag(KLVMFunc *fn, KLVMValue *val)
{
    if (val->tag < 0) { val->tag = fn->tag_index++; }
    return val->tag;
}

static inline int bb_tag(KLVMBasicBlock *bb)
{
    if (bb->tag < 0) { bb->tag = ((KLVMFunc *)bb->fn)->bb_tag_index++; }
    return bb->tag;
}

static void inst_show_value(KLVMFunc *fn, KLVMValue *val)
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
                if (var->name && var->name[0])
                    printf("%%%s", var->name);
                else {
                    printf("%%%d", val_tag(fn, val));
                }
            }
            break;
        }
        case VALUE_INST: {
            KLVMInst *inst = (KLVMInst *)val;
            if (inst->name && inst->name[0])
                printf("%%%s", inst->name);
            else
                printf("%%%d", val_tag(fn, val));
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
    printf(" %s", KLVMTypeString(ty));
}

static void inst_binary_show(KLVMFunc *fn, KLVMBinaryInst *inst)
{
    if (inst->name && inst->name[0])
        printf("%%%s", inst->name);
    else
        printf("%%%d", val_tag(fn, (KLVMValue *)inst));

    type_show(inst->type);

    printf(" = ");

    switch (inst->bop) {
        case KLVM_BINARY_ADD:
            printf("add ");
            break;
        case KLVM_BINARY_SUB:
            printf("sub ");
            break;
        case KLVM_BINARY_CMP_LE:
            printf("cmple ");
            break;
        default:
            printf("error:invalid binary instruction\n");
            abort();
            break;
    }

    KLVMValue *lhs = ((KLVMBinaryInst *)inst)->lhs;
    inst_show_value(fn, lhs);
    type_show(lhs->type);
    printf(", ");
    KLVMValue *rhs = ((KLVMBinaryInst *)inst)->rhs;
    inst_show_value(fn, rhs);
    type_show(rhs->type);
}

static void KLVMInstShow(KLVMFunc *fn, KLVMInst *inst)
{
    switch (inst->op) {
        case KLVM_INST_COPY: {
            KLVMCopyInst *cp = (KLVMCopyInst *)inst;
            inst_show_value(fn, cp->lhs);
            type_show(cp->type);
            printf(" = ");
            inst_show_value(fn, cp->rhs);
            type_show(cp->rhs->type);
            break;
        }
        case KLVM_INST_BINARY:
            inst_binary_show(fn, (KLVMBinaryInst *)inst);
            break;
        case KLVM_INST_CALL: {
            KLVMCallInst *call = (KLVMCallInst *)inst;
            if (inst->name && inst->name[0])
                printf("%%%s", inst->name);
            else
                printf("%%%d", val_tag(fn, (KLVMValue *)inst));

            type_show(call->type);

            printf(" = call ");

            inst_show_value(fn, call->fn);
            printf("(");
            KLVMValue **val;
            vector_foreach(val, &call->args)
            {
                if (i != 0) printf(", ");
                inst_show_value(fn, *val);
                type_show((*val)->type);
            }
            printf(")");
            break;
        }
        case KLVM_INST_RET: {
            KLVMRetInst *ret = (KLVMRetInst *)inst;
            printf("ret ");
            inst_show_value(fn, ret->ret);
            type_show(ret->type);
            break;
        }
        case KLVM_INST_JMP_COND: {
            printf("br ");
            KLVMCondJmpInst *br = (KLVMCondJmpInst *)inst;
            inst_show_value(fn, br->cond);
            type_show(br->cond->type);

            KLVMBasicBlock *bb = br->_then;
            if (bb->label && bb->label[0])
                printf(", label %%%s", bb->label);
            else
                printf(", label %%bb%d", bb_tag(bb));

            bb = br->_else;
            if (bb->label && bb->label[0])
                printf(", label %%%s", bb->label);
            else
                printf(", label %%bb%d", bb_tag(bb));
            break;
        }
        case KLVM_INST_JMP: {
            printf("br ");
            KLVMJmpInst *br = (KLVMJmpInst *)inst;
            KLVMBasicBlock *bb = br->dest;
            if (bb->label && bb->label[0])
                printf("label %%%s", bb->label);
            else
                printf("label %%bb%d", bb_tag(bb));
            break;
        }
        default:
            break;
    }
}

static void KLVMBasicBlockShow(KLVMBasicBlock *bb, int indent)
{
    if (bb->label && bb->label[0])
        printf("%%%s:\n", bb->label);
    else
        printf("%%bb%d:\n", bb_tag(bb));
    KLVMInst *inst;
    ListNode *n;
    list_foreach(&bb->insts, n)
    {
        inst = container_of(n, KLVMInst, node);
        if (indent) printf("  ");
        KLVMInstShow((KLVMFunc *)bb->fn, inst);
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
        if (para->name && para->name[0])
            strcat(buf, para->name);
        else {
            int l = strlen(buf);
            sprintf(buf + l, "%%%d", val_tag(fn, (KLVMValue *)para));
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
