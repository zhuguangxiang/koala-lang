/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_LLVM_H_
#define _KOALA_LLVM_H_

#include "codeobject.h"
#include "typedesc.h"
#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* koala llvm type */
typedef struct {
    TypeDesc *type;
    LLVMTypeRef lltype;
} kl_llvm_type_t;

/* koala llvm value */
typedef struct {
    kl_llvm_type_t type;
    LLVMValueRef llvalue;
} kl_llvm_value_t;

/* koala llvm function */
typedef struct {
    CodeObject *co;
    Vector ptypes;
    kl_llvm_type_t rtype;
    Vector locals;
    LLVMValueRef llfunc;
    int blk;
    Vector blocks;
} kl_llvm_func_t;

/* translation context */
typedef struct kl_llvm_ctx {
    LLVMModuleRef llmod;
    TypeObject *mo;
    Vector stack;
    kl_llvm_func_t *func;
    LLVMBuilderRef builder;
} kl_llvm_ctx_t;

/*
  A LLVM IR basic block. A basic block is a sequence of instructions
  whose execution always goes from start to end. That is, a control flow
  instruction (branch) can only appear as the last instruction, and
  incoming branches can only jump to the first instruction.
 */
typedef struct kl_llvm_block {
    char *label;
    int index;
    kl_llvm_ctx_t *ctx;
    LLVMBasicBlockRef bb;
    int start;
    int end;
    struct kl_llvm_block *parent;
    Vector children;
} kl_llvm_block_t;

/* fini translation context */
void kl_llvm_fini_ctx(kl_llvm_ctx_t *ctx);

/* translate */
void kl_llvm_translate(kl_llvm_ctx_t *ctx, TypeObject *mo);

/* generate native object(.o) file */
void kl_llvm_gen_native(kl_llvm_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LLVM_H_ */
