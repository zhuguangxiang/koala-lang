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

#include "klvm/klvm_insn.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*\
|* Builder                                                                  *|
\*--------------------------------------------------------------------------*/

void KLVMSetBuilder(KLVMBuilderRef bldr, KLVMValueRef _insn)
{
    KLVMInsnRef insn = (KLVMInsnRef)_insn;
    bldr->it = &insn->link;
}

void KLVMSetBuilderBefore(KLVMBuilderRef bldr, KLVMValueRef _insn)
{
    KLVMInsnRef insn = (KLVMInsnRef)_insn;
    bldr->it = insn->link.prev;
}

/*--------------------------------------------------------------------------*\
|* Use                                                                      *|
\*--------------------------------------------------------------------------*/

static void InitUse(KLVMUseRef use, KLVMInsnRef insn, KLVMIntervalRef interval)
{
    use->insn = insn;

    InitList(&use->use_link);
    ListAdd(&interval->use_list, &use->use_link);
    use->interval = interval;
}

/*--------------------------------------------------------------------------*\
|* Instruction                                                              *|
\*--------------------------------------------------------------------------*/

static inline void InsnAppend(KLVMBuilderRef bldr, KLVMInsnRef insn)
{
    ListAdd(bldr->it, &insn->link);
    bldr->it = &insn->link;
    bldr->bb->num_insns++;
}

static KLVMInsnRef InsnAlloc(KLVMInsnKind op, int num_ops, KLVMTypeRef type,
                             char *name)
{
    KLVMInsnRef insn = MemAlloc(sizeof(*insn) + sizeof(KLVMOper) * num_ops);
    if (insn) {
        INIT_INSN_HEAD(insn, type, op, name);
        insn->num_ops = num_ops;
    }
    return insn;
}

static void InsnFree(KLVMInsnRef insn)
{
    KLVMUseRef use;
    for (int i = 0; i < insn->num_ops; i++) {
        use = &insn->operands[i].use;
        ListRemove(&use->use_link);
    }
    MemFree(insn);
}

static void InitRegOper(KLVMOperRef oper, KLVMInsnRef insn, KLVMValueRef ref)
{
    assert(ref->kind == KLVM_VALUE_VAR || ref->kind == KLVM_VALUE_INSN);
    oper->kind = KLVM_OPER_USE;
    if (!ref->interval) ref->interval = IntervalAlloc(ref);
    InitUse(&oper->use, insn, ref->interval);
}

static void InitConstOper(KLVMOperRef oper, KLVMConstRef konst)
{
    oper->kind = KLVM_OPER_CONST;
    oper->konst = konst;
}

static void InitOper(KLVMOperRef oper, KLVMInsnRef insn, KLVMValueRef ref)
{
    // constant, variable or instruction
    assert(ref->kind == KLVM_VALUE_VAR || ref->kind == KLVM_VALUE_CONST ||
           ref->kind == KLVM_VALUE_INSN);

    if (ref->kind == KLVM_VALUE_CONST)
        InitConstOper(oper, (KLVMConstRef)ref);
    else
        InitRegOper(oper, insn, ref);
}

static void InitFuncOper(KLVMOperRef oper, KLVMValueRef fn)
{
    oper->kind = KLVM_OPER_FUNC;
    oper->fn = fn;
}

static void InitBlockOper(KLVMOperRef oper, KLVMBasicBlockRef block)
{
    oper->kind = KLVM_OPER_BLOCK;
    oper->block = block;
}

void KLVMBuildCopy(KLVMBuilderRef bldr, KLVMValueRef lhs, KLVMValueRef rhs)
{
    if (!KLVMTypeEqual(lhs->type, rhs->type)) {
        printf("error: type not matched\n");
    }

    KLVMInsnRef insn = InsnAlloc(KLVM_INSN_COPY, 2, KLVMTypeNone(), "");

    // lhs must be variable
    assert(lhs->kind == KLVM_VALUE_VAR);
    InitRegOper(&insn->operands[0], insn, lhs);

    // rhs maybe constant, variable or instruction
    InitOper(&insn->operands[1], insn, rhs);

    InsnAppend(bldr, insn);
}

static int _IsCmp(KLVMInsnKind op)
{
    static KLVMInsnKind ops[] = {
        KLVM_INSN_CMP_EQ, KLVM_INSN_CMP_NE, KLVM_INSN_CMP_GT,
        KLVM_INSN_CMP_GE, KLVM_INSN_CMP_LT, KLVM_INSN_CMP_LE,
    };

    for (int i = 0; i < COUNT_OF(ops); i++) {
        if (op == ops[i]) return 1;
    }

    return 0;
}

KLVMValueRef KLVMBuildBinary(KLVMBuilderRef bldr, KLVMInsnKind op,
                             KLVMValueRef lhs, KLVMValueRef rhs, char *name)
{
    KLVMTypeRef type = _IsCmp(op) ? KLVMTypeBool() : lhs->type;
    KLVMInsnRef insn = InsnAlloc(op, 2, type, name);

    InitOper(&insn->operands[0], insn, lhs);
    InitOper(&insn->operands[1], insn, rhs);

    KLVMFuncRef _fn = (KLVMFuncRef)bldr->bb->fn;
    insn->reg = _fn->regs++;

    InsnAppend(bldr, insn);
    return (KLVMValueRef)insn;
}

KLVMValueRef KLVMBuildCall(KLVMBuilderRef bldr, KLVMValueRef fn,
                           KLVMValueRef args[], int size, char *name)
{
    KLVMFuncRef _fn = (KLVMFuncRef)fn;
    KLVMTypeRef type = KLVMProtoRet(_fn->type);
    KLVMInsnRef insn = InsnAlloc(KLVM_INSN_CALL, size + 1, type, name);

    InitFuncOper(&insn->operands[0], fn);
    KLVMValueRef arg;
    for (int i = 0; i < size; i++) {
        arg = args[i];
        InitOper(&insn->operands[i + 1], insn, arg);
    }

    if (type != KLVMTypeNone()) {
        insn->reg = _fn->regs++;
    }

    InsnAppend(bldr, insn);
    return (KLVMValueRef)insn;
}

void KLVMBuildJmp(KLVMBuilderRef bldr, KLVMBasicBlockRef dst)
{
    KLVMInsnRef insn = InsnAlloc(KLVM_INSN_JMP, 1, KLVMTypeNone(), "");
    InitBlockOper(&insn->operands[0], dst);
    InsnAppend(bldr, insn);
    KLVMLinkEdge(bldr->bb, dst);
}

void KLVMBuildCondJmp(KLVMBuilderRef bldr, KLVMValueRef cond,
                      KLVMBasicBlockRef _then, KLVMBasicBlockRef _else)
{
    KLVMInsnRef insn = InsnAlloc(KLVM_INSN_COND_JMP, 3, KLVMTypeNone(), "");
    InitOper(&insn->operands[0], insn, cond);
    InitBlockOper(&insn->operands[1], _then);
    InitBlockOper(&insn->operands[2], _else);
    InsnAppend(bldr, insn);
    KLVMLinkEdge(bldr->bb, _then);
    KLVMLinkEdge(bldr->bb, _else);
}

void KLVMBuildRet(KLVMBuilderRef bldr, KLVMValueRef v)
{
    KLVMInsnRef insn = InsnAlloc(KLVM_INSN_RET, 1, KLVMTypeNone(), "");
    InitOper(&insn->operands[0], insn, v);
    InsnAppend(bldr, insn);
    KLVMFuncRef fn = (KLVMFuncRef)bldr->bb->fn;
    KLVMLinkEdge(bldr->bb, fn->ebb);
}

void KLVMBuildRetVoid(KLVMBuilderRef bldr)
{
    KLVMInsnRef insn = InsnAlloc(KLVM_INSN_RET, 0, KLVMTypeNone(), "");
    InsnAppend(bldr, insn);
    KLVMFuncRef fn = (KLVMFuncRef)bldr->bb->fn;
    KLVMLinkEdge(bldr->bb, fn->ebb);
}

#ifdef __cplusplus
}
#endif
