/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_JIT_LLVM_H_
#define _KOALA_JIT_LLVM_H_

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jittype {
  LLVMTypeRef lltype;
  char kind;
} JitType;

#define JIT_VALUE_HEAD  \
  LLVMValueRef llvalue; \
  int64_t raw; \
  JitType type;

typedef struct jitvalue {
  JIT_VALUE_HEAD
} JitValue;

typedef struct jitvariable {
  JIT_VALUE_HEAD
  char *name;
} JitVariable;

typedef struct jitfunction {
  char *name;
  GVector argtypes;
  JitType rettype;
  LLVMTypeRef llproto;
  LLVMValueRef llfunc;
} JitFunction;

typedef struct jitcontext {
  CodeObject *co;
  JitFunction *func;
  Vector locals;
  GVector stack;
  int curblk;
  Vector blocks;
  LLVMBuilderRef builder;
} JitContext;

/*
  A LLVM IR basic block. A basic block is a sequence of instructions
  whose execution always goes from start to end. That is, a control flow
  instruction (branch) can only appear as the last instruction, and
  incoming branches can only jump to the first instruction.
 */
typedef struct jitblock {
  char *label;
  int index;
  JitContext *ctx;
  LLVMBasicBlockRef bb;
  int start;
  int end;
  struct jitblock *parent;
  Vector children;
} JitBlock;

void init_jit_llvm(void);
void fini_jit_llvm(void);

static inline LLVMTypeRef LLVMPtrType(LLVMTypeRef subtype)
{
  return LLVMPointerType(subtype, 0);
}

static inline LLVMTypeRef LLVMStringType(void)
{
  return LLVMPtrType(LLVMInt8Type());
}

static inline LLVMTypeRef LLVMVoidPtrType(void)
{
  return LLVMPtrType(LLVMVoidType());
}

static inline LLVMTypeRef LLVMVoidPtrPtrType(void)
{
  return LLVMPtrType(LLVMVoidPtrType());
}

static inline LLVMTypeRef LLVMIntPtrType2(void)
{
  return sizeof(void *) == 8 ? LLVMInt64Type() : LLVMInt32Type();
}

void jit_context_init(JitContext *ctx, CodeObject *co);
void jit_context_fini(JitContext *ctx);
void *jit_emit_code(JitContext *ctx);
void jit_verify_ir(void);
void jit_optimize(void);

/*
JitType jit_type(TypeDesc *desc);
JitVariable *jit_variable(char *name, JitType type);
void jit_free_variable(JitVariable *var);
*/

JitBlock *jit_block(JitContext *ctx, char *label);
void jit_free_block(JitBlock *blk);

void jit_OP_POP_TOP(JitContext *ctx);
void jit_OP_LOAD_CONST(JitContext *ctx, int index);
void jit_OP_LOAD(JitContext *ctx, int index);
void jit_OP_STORE(JitContext *ctx, int index);
void jit_OP_GET_VALUE(JitContext *ctx, int index);
void jit_OP_SET_VALUE(JitContext *ctx, int index);
void jit_OP_RETURN_VALUE(JitContext *ctx);
void jit_OP_RETURN(JitContext *ctx);
void jit_OP_CALL(JitContext *ctx, int index, int count);
void jit_OP_ADD(JitContext *ctx);
void jit_OP_NEW_TUPLE(JitContext *ctx, int count);
void jit_OP_NEW_ARRAY(JitContext *ctx, int index, int count);
void jit_OP_NEW_MAP(JitContext *ctx, int index, int count);
void jit_OP_NEW(JitContext *ctx, int index);
void jit_OP_LOAD_GLOBAL(JitContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_LLVM_H_ */
