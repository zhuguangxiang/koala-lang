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

#ifndef _KLVM_INSN_H_
#define _KLVM_INSN_H_

#if !defined(_KLVM_H_INSIDE_) && !defined(KLVM_COMPILATION)
#error "Only <klvm.h> can be included directly."
#endif

#include "klvm/klvm_value.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLVMInsnKind {
    KLVM_INSN_NONE,

    KLVM_INSN_COPY,

    KLVM_INSN_ADD,
    KLVM_INSN_SUB,
    KLVM_INSN_MUL,
    KLVM_INSN_DIV,
    KLVM_INSN_MOD,
    KLVM_INSN_POW,

    KLVM_INSN_CMP_EQ,
    KLVM_INSN_CMP_NE,
    KLVM_INSN_CMP_GT,
    KLVM_INSN_CMP_GE,
    KLVM_INSN_CMP_LT,
    KLVM_INSN_CMP_LE,

    KLVM_INSN_AND,
    KLVM_INSN_OR,

    KLVM_INSN_BIT_AND,
    KLVM_INSN_BIT_OR,
    KLVM_INSN_BIT_XOR,
    KLVM_INSN_BIT_SHL,
    KLVM_INSN_BIT_SHR,

    KLVM_INSN_NEG,
    KLVM_INSN_NOT,
    KLVM_INSN_BIT_NOT,

    KLVM_INSN_CALL,

    KLVM_INSN_RET,
    KLVM_INSN_RET_VOID,
    KLVM_INSN_JMP,
    KLVM_INSN_COND_JMP,

    KLVM_INSN_INDEX,
    KLVM_INSN_NEW,
    KLVM_INSN_FIELD,
    KLVM_INSN_MAX,
} KLVMInsnKind;

// clang-format off

#define INIT_INSN_HEAD(insn, type, _op, _name) \
    INIT_VALUE_HEAD(insn, KLVM_VALUE_INSN, type, _name); \
    insn->op = _op; InitList(&insn->link);

// clang-format on

typedef struct _KLVMInsn KLVMInsn, *KLVMInsnRef;

typedef struct _KLVMUse {
    // link in KLVMInterval
    List use_link;
    // live interval
    KLVMIntervalRef interval;
    // self instruction
    KLVMInsnRef insn;
} KLVMUse, *KLVMUseRef;

#define UseForEach(use, use_list, closure) \
    ListForEach(use, use_link, use_list, closure)

typedef enum _KLVMOperKind {
    KLVM_OPER_NONE,
    KLVM_OPER_CONST,
    KLVM_OPER_USE,
    KLVM_OPER_FUNC,
    KLVM_OPER_BLOCK,
    KLVM_OPER_MAX,
} KLVMOperKind;

typedef struct _KLVMOper {
    KLVMOperKind kind;
    union {
        KLVMConstRef konst;
        KLVMUse use;
        KLVMValueRef fn;
        KLVMBasicBlockRef block;
    };
} KLVMOper, *KLVMOperRef;

struct _KLVMInsn {
    KLVM_VALUE_HEAD
    KLVMInsnKind op;
    List link;
    int pos;
    int num_ops;
    KLVMOper operands[0];
};

/* instruction iteration */
#define InsnForEach(insn, insn_list, closure) \
    ListForEach(insn, link, insn_list, closure)

void KLVMPrintInsn(KLVMFuncRef fn, KLVMInsnRef insn, FILE *fp);

void KLVMBuildCopy(KLVMBuilderRef bldr, KLVMValueRef lhs, KLVMValueRef rhs);

KLVMValueRef KLVMBuildBinary(KLVMBuilderRef bldr, KLVMInsnKind op,
                             KLVMValueRef lhs, KLVMValueRef rhs, char *name);
#define KLVMBuildAdd(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_INSN_ADD, lhs, rhs, name)
#define KLVMBuildSub(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_INSN_SUB, lhs, rhs, name)

#define KLVMBuildCmple(bldr, lhs, rhs, name) \
    KLVMBuildBinary(bldr, KLVM_INSN_CMP_LE, lhs, rhs, name)

KLVMValueRef KLVMBuildCall(KLVMBuilderRef bldr, KLVMValueRef fn,
                           KLVMValueRef args[], int size, char *name);

void KLVMBuildJmp(KLVMBuilderRef bldr, KLVMBasicBlockRef dst);
void KLVMBuildCondJmp(KLVMBuilderRef bldr, KLVMValueRef cond,
                      KLVMBasicBlockRef _then, KLVMBasicBlockRef _else);

void KLVMBuildRet(KLVMBuilderRef bldr, KLVMValueRef ret);
void KLVMBuildRetVoid(KLVMBuilderRef bldr);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_INSN_H_ */
