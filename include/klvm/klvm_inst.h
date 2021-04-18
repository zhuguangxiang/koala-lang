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

#ifndef _KLVM_INST_H_
#define _KLVM_INST_H_

#if !defined(_KLVM_H_INSIDE_) && !defined(KLVM_COMPILATION)
#error "Only <klvm.h> can be included directly."
#endif

#include "klvm/klvm_value.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLVMInstKind {
    KLVM_INST_INVALID,

    KLVM_INST_COPY,

    KLVM_INST_ADD,
    KLVM_INST_SUB,
    KLVM_INST_MUL,
    KLVM_INST_DIV,
    KLVM_INST_MOD,
    KLVM_INST_POW,

    KLVM_INST_CMP_EQ,
    KLVM_INST_CMP_NE,
    KLVM_INST_CMP_GT,
    KLVM_INST_CMP_GE,
    KLVM_INST_CMP_LT,
    KLVM_INST_CMP_LE,

    KLVM_INST_AND,
    KLVM_INST_OR,

    KLVM_INST_BIT_AND,
    KLVM_INST_BIT_OR,
    KLVM_INST_BIT_XOR,
    KLVM_INST_BIT_SHL,
    KLVM_INST_BIT_SHR,

    KLVM_INST_NEG,
    KLVM_INST_NOT,
    KLVM_INST_BIT_NOT,

    KLVM_INST_CALL,

    KLVM_INST_RET,
    KLVM_INST_RET_VOID,
    KLVM_INST_JMP,
    KLVM_INST_JMP_COND,

    KLVM_INST_INDEX,
    KLVM_INST_NEW,
    KLVM_INST_FIELD,
} KLVMInstKind;

// clang-format off

#define KLVM_INST_HEAD KLVM_VALUE_HEAD List link; KLVMInstKind op;

#define INIT_KLVM_INST_HEAD(inst, type, _op, _name) \
    INIT_KLVM_VALUE_HEAD(inst, KLVM_VALUE_INST, type, _name); \
    InitList(&inst->link); inst->op = _op

// clang-format on

typedef struct _KLVMInst {
    KLVM_INST_HEAD
} KLVMInst, *KLVMInstRef;

/* Print Instruction */
void KLVMPrintInst(KLVMFuncRef fn, KLVMInstRef inst, FILE *fp);

typedef struct _KLVMCopyInst {
    KLVM_INST_HEAD
    KLVMValueRef lhs;
    KLVMValueRef rhs;
} KLVMCopyInst, *KLVMCopyInstRef;

typedef struct _KLVMBinaryInst {
    KLVM_INST_HEAD
    KLVMValueRef lhs;
    KLVMValueRef rhs;
} KLVMBinaryInst, *KLVMBinaryInstRef;

typedef struct _KLVMUnaryInst {
    KLVM_INST_HEAD
    KLVMValueRef val;
} KLVMUnaryInst, *KLVMUnaryInstRef;

typedef struct _KLVMCallInst {
    KLVM_INST_HEAD
    KLVMValueRef fn;
    Vector args;
} KLVMCallInst, *KLVMCallInstRef;

typedef struct _KLVMJmpInst {
    KLVM_INST_HEAD
    KLVMBlockRef dest;
} KLVMJmpInst, *KLVMJmpInstRef;

typedef struct _KLVMBranchInst {
    KLVM_INST_HEAD
    KLVMValueRef cond;
    KLVMBlockRef _then;
    KLVMBlockRef _else;
} KLVMCondJmpInst, *KLVMCondJmpInstRef;

typedef struct _KLVMRetInst {
    KLVM_INST_HEAD
    KLVMValueRef ret;
} KLVMRetInst, *KLVMRetInstRef;

void KLVMBuildCopy(KLVMBuilderRef bldr, KLVMValueRef lhs, KLVMValueRef rhs);

KLVMValueRef KLVMBuildBinary(KLVMBuilderRef bldr, KLVMInstKind op,
                             KLVMValueRef lhs, KLVMValueRef rhs, char *name);
#define KLVMBuildAdd(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_INST_ADD, lhs, rhs, name)
#define KLVMBuildSub(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_INST_SUB, lhs, rhs, name)

#define KLVMBuildCmple(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_INST_CMP_LE, lhs, rhs, name)

KLVMValueRef KLVMBuildCall(KLVMBuilderRef bldr, KLVMValueRef fn,
                           KLVMValueRef args[], int size, char *name);

void KLVMBuildJmp(KLVMBuilderRef bldr, KLVMBlockRef dst);
void KLVMBuildCondJmp(KLVMBuilderRef bldr, KLVMValueRef cond,
                      KLVMBlockRef _then, KLVMBlockRef _else);

void KLVMBuildRet(KLVMBuilderRef bldr, KLVMValueRef ret);
void KLVMBuildRetVoid(KLVMBuilderRef bldr);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_INST_H_ */
