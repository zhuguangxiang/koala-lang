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

/*--------------------------------------------------------------------------*\
|* Instruction Builder                                                      *|
\*--------------------------------------------------------------------------*/

void KLVMSetBuilder(KLVMBuilderRef bldr, KLVMValueRef _inst)
{
    KLVMInstRef inst = (KLVMInstRef)_inst;
    bldr->it = &inst->link;
}

void KLVMSetBuilderBefore(KLVMBuilderRef bldr, KLVMValueRef _inst)
{
    KLVMInstRef inst = (KLVMInstRef)_inst;
    bldr->it = inst->link.prev;
}

static inline void __inst_append(KLVMBuilderRef bldr, KLVMInstRef inst)
{
    ListAdd(bldr->it, &inst->link);
    bldr->it = &inst->link;
    bldr->bb->num_insts++;
}

void KLVMBuildCopy(KLVMBuilderRef bldr, KLVMValueRef lhs, KLVMValueRef rhs)
{
    if (!KLVMTypeEqual(lhs->type, rhs->type)) {
        printf("error: type not matched\n");
    }

    KLVMCopyInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, lhs->type, KLVM_INST_COPY, "");
    inst->lhs = lhs;
    inst->rhs = rhs;

    __inst_append(bldr, (KLVMInstRef)inst);
}

static int __is_cmp_inst(KLVMInstKind op)
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

KLVMValueRef KLVMBuildBinary(KLVMBuilderRef bldr, KLVMInstKind op,
                             KLVMValueRef lhs, KLVMValueRef rhs, char *name)
{
    KLVMBinaryInstRef inst = MemAllocWithPtr(inst);

    if (__is_cmp_inst(op)) {
        INIT_KLVM_INST_HEAD(inst, KLVMTypeBool(), op, name);
    } else {
        INIT_KLVM_INST_HEAD(inst, lhs->type, op, name);
    }

    inst->lhs = lhs;
    inst->rhs = rhs;
    __inst_append(bldr, (KLVMInstRef)inst);
    return (KLVMValueRef)inst;
}

KLVMValueRef KLVMBuildCall(KLVMBuilderRef bldr, KLVMValueRef fn,
                           KLVMValueRef args[], int size, char *name)
{
    KLVMCallInstRef inst = MemAllocWithPtr(inst);
    KLVMTypeRef rty = KLVMProtoRet(((KLVMFuncRef)fn)->type);
    INIT_KLVM_INST_HEAD(inst, rty, KLVM_INST_CALL, name);
    inst->fn = fn;
    VectorInitPtr(&inst->args);

    KLVMValueRef *arg;
    for (int i = 0; i < size; i++) {
        arg = args + i;
        VectorPushBack(&inst->args, arg);
    }

    __inst_append(bldr, (KLVMInstRef)inst);
    return (KLVMValueRef)inst;
}

void KLVMBuildJmp(KLVMBuilderRef bldr, KLVMBlockRef dst)
{
    KLVMJmpInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, NULL, KLVM_INST_JMP, "");
    inst->dest = dst;
    __inst_append(bldr, (KLVMInstRef)inst);

    // add edge
    KLVMLinkEdge(bldr->bb, dst);
}

void KLVMBuildCondJmp(KLVMBuilderRef bldr, KLVMValueRef cond,
                      KLVMBlockRef _then, KLVMBlockRef _else)
{
    KLVMCondJmpInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, NULL, KLVM_INST_JMP_COND, "");
    inst->cond = cond;
    inst->_then = _then;
    inst->_else = _else;
    __inst_append(bldr, (KLVMInstRef)inst);

    // add edges
    KLVMLinkEdge(bldr->bb, _then);
    KLVMLinkEdge(bldr->bb, _else);
}

void KLVMBuildRet(KLVMBuilderRef bldr, KLVMValueRef v)
{
    KLVMRetInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, v->type, KLVM_INST_RET, "");
    inst->ret = v;
    __inst_append(bldr, (KLVMInstRef)inst);

    // add end edge
    KLVMFuncRef fn = (KLVMFuncRef)bldr->bb->fn;
    KLVMLinkEdge(bldr->bb, fn->ebb);
}

void KLVMBuildRetVoid(KLVMBuilderRef bldr)
{
    KLVMInstRef inst = MemAllocWithPtr(inst);
    INIT_KLVM_INST_HEAD(inst, NULL, KLVM_INST_RET_VOID, "");
    __inst_append(bldr, inst);

    // add end edge
    KLVMFuncRef fn = (KLVMFuncRef)bldr->bb->fn;
    KLVMLinkEdge(bldr->bb, fn->ebb);
}

#ifdef __cplusplus
}
#endif
