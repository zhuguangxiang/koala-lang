/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include <inttypes.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef kl_gen_func(LLVMModuleRef M, char *name, LLVMTypeRef fn_ty)
{
    return LLVMAddFunction(M, name, fn_ty);
}

void kl_gen_pointer_new(LLVMModuleRef M, char *name, LLVMTypeRef ety)
{
    LLVMTypeRef fn_ty =
        LLVMFunctionType(LLVMPointerType(ety, 0), param_types, 0, 0);

    LLVMValueRef fn = LLVMAddFunction(M, "Pointer$Int8$new", fn_ty);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fn, "");
    LLVMBuilderRef bldr = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(bldr, entry);
    LLVMValueRef ret = LLVMBuildAlloca(bldr, LLVMPointerType(ety, 0), "");
    LLVMBuildRet(bldr, ret);
    LLVMDisposeBuilder(bldr);
}

LLVMValueRef kl_gen_pointer_new(LLVMBuilderRef bldr, LLVMValueRef val)
{
    LLVMTypeRef pt_ty = LLVMPointerType(LLVMTypeOf(val), 0);
    LLVMValueRef ptr = LLVMBuildAlloca(bldr, pt_ty, "");
    LLVMBuildStore(bldr, val, ptr);
    return ptr;
}
