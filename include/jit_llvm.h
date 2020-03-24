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
//#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
//#include <llvm-c/BitWriter.h>

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct llvmfunction {
  char *name;
  LLVMTypeRef proto;
  LLVMValueRef llfunc;
} LLVMFunction;

typedef struct jitstate {
  LLVMExecutionEngineRef engine;
  LLVMModuleRef mod;
  LLVMFunction call;
  LLVMFunction decref;
  LLVMFunction incref;
  LLVMFunction initargs;
  LLVMFunction clearstk;
  HashMap map;
} JITState;

typedef struct jittype {
  LLVMTypeRef lltype;
  int basic;
  char *str;
} JITType;

typedef struct jitvalue {
  LLVMValueRef llvalue;
  JITType type;
  int konst;
  char *name;
} JITValue;

typedef struct jitfunction {
  CodeObject *co;
  GVector argtypes;
  JITType rettype;
  LLVMFunction func;
} JITFunction;

/*
  A LLVM IR basic block. A basic block is a sequence of instructions
  whose execution always goes from start to end. That is, a control flow
  instruction (branch) can only appear as the last instruction, and
  incoming branches can only jump to the first instruction.
 */
typedef struct jitblock {
  char *label;
  int index;
  JITFunction *func;
  LLVMBasicBlockRef bb;
  int start;
  int end;
  struct jitblock *parent;
  Vector children;
} JITBlock;

typedef struct jitcontext {
  Vector locals;
  Vector stack;
  JITFunction *func;
  int curblk;
  Vector blocks;
  LLVMBuilderRef builder;
} JITContext;

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

void jit_context_init(JITContext *ctx, JITFunction *f);
void jit_context_fini(JITContext *ctx);

JITType jit_type(TypeDesc *desc);
LLVMValueRef jit_const(Object *ob);
JITValue *jit_value(char *name, JITType type);
void jit_free_value(JITValue *val);
void jit_store_value(JITContext *ctx, JITValue *var, LLVMValueRef val);

LLVMFunction llvm_function(char *name, LLVMTypeRef proto);
LLVMFunction llvm_cfunction(char *name, LLVMTypeRef proto, void *addr);
JITFunction *jit_function(CodeObject *co);
void *jit_emit_code(JITFunction *func);
void jit_free_function(JITFunction *func);

JITBlock *jit_block(JITContext *ctx, char *label);
void jit_free_block(JITBlock *blk);

void jit_verify_ir(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_LLVM_H_ */
