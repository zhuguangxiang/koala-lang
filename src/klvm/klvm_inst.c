/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "klvm/klvm_inst.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void __InstAppend(KLVMVisitorRef vst, KLVMInstRef inst)
{
    if (!vst->it)
        ListPushBack(&vst->bb->insts, &inst->node);
    else
        ListAddAfter(&inst->node, vst->it);
    vst->it = &inst->node;
    vst->bb->num_insts++;
}

void KLVMInstCopy(KLVMVisitorRef vst, KLVMValueRef lhs, KLVMValueRef rhs)
{
    if (!KLVMTypeEqual(lhs->type, rhs->type)) {
        printf("error: type not matched\n");
    }

    KLVMCopyInst *inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, lhs->type, KLVM_INST_COPY, NULL);
    inst->lhs = lhs;
    inst->rhs = rhs;

    __InstAppend(vst, (KLVMInstRef)inst);
}

static int __IsCmpInst(KLVMInstKind op)
{
    static KLVMInstKind cmps[] = {
        KLVM_INST_CMP_EQ, KLVM_INST_CMP_NE, KLVM_INST_CMP_GT,
        KLVM_INST_CMP_GE, KLVM_INST_CMP_LT, KLVM_INST_CMP_LE,
    };

    for (int i = 0; i < COUNT_OF(cmps); i++) {
        if (op == cmps[i]) return 1;
    }

    return 0;
}

KLVMValueRef KLVMInstBinary(KLVMVisitorRef vst, KLVMInstKind op,
                            KLVMValueRef lhs, KLVMValueRef rhs, char *name)
{
    KLVMBinaryInstRef inst = MemAllocWithPtr(inst);

    if (__IsCmpInst(op)) {
        INIT_KLVM_INST_HEAD(inst, KLVMTypeBool(), op, name);
    } else {
        INIT_KLVM_INST_HEAD(inst, lhs->type, op, name);
    }

    inst->lhs = lhs;
    inst->rhs = rhs;
    __InstAppend(vst, (KLVMInstRef)inst);
    return (KLVMValueRef)inst;
}

KLVMValueRef KLVMInstCall(KLVMVisitorRef vst, KLVMValueRef fn,
                          KLVMValueRef *args, char *name)
{
    KLVMCallInstRef inst = MemAllocWithPtr(inst);
    KLVMTypeRef rty = KLVMProtoRet(((KLVMFuncRef)fn)->type);
    INIT_KLVM_INST_HEAD(inst, rty, KLVM_INST_CALL, name);
    inst->fn = fn;
    VectorInit(&inst->args, sizeof(void *));

    KLVMValueRef *arg = args;
    while (*arg) {
        VectorPushBack(&inst->args, arg);
        arg++;
    }

    __InstAppend(vst, (KLVMInstRef)inst);
    return (KLVMValueRef)inst;
}

void KLVMInstJmp(KLVMVisitorRef vst, KLVMBlockRef dest)
{
    KLVMJmpInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, NULL, KLVM_INST_JMP, NULL);
    inst->dest = dest;
    __InstAppend(vst, (KLVMInstRef)inst);
}

void KLVMInstCondJmp(KLVMVisitorRef vst, KLVMValueRef cond, KLVMBlockRef _then,
                     KLVMBlockRef _else)
{
    KLVMCondJmpInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, NULL, KLVM_INST_JMP_COND, NULL);
    inst->cond = cond;
    inst->_then = _then;
    inst->_else = _else;
    __InstAppend(vst, (KLVMInstRef)inst);
}

void KLVMInstRet(KLVMVisitorRef vst, KLVMValueRef v)
{
    KLVMRetInst *inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, v->type, KLVM_INST_RET, NULL);
    inst->ret = v;
    __InstAppend(vst, (KLVMInstRef)inst);
}

void KLVMInstRetVoid(KLVMVisitorRef vst)
{
    KLVMInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, NULL, KLVM_INST_RET_VOID, NULL);
    __InstAppend(vst, (KLVMInstRef)inst);
}

#ifdef __cplusplus
}
#endif
