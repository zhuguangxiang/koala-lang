
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

/*
cc test_gc.c -o test_gc -std=gnu17 -I/usr/lib/llvm-11/include  -D_GNU_SOURCE \
-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -lLLVM-11

cc test_gc_readonly.c -o test_gc_ro -std=gnu17 -I/home/james/.local/include
-D_GNU_SOURCE \ -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS
-D__STDC_LIMIT_MACROS -lLLVM-13
*/

int main(int argc, char *argv[])
{
    LLVMModuleRef mod = LLVMModuleCreateWithName("test_gc");

    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8Type(), 1) };

    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt8Type(), param_types, 1, 0);
    LLVMValueRef foo = LLVMAddFunction(mod, "foo", ret_type);
    LLVMSetGC(foo, "statepoint-example");
    // LLVMSetSection(foo, ".init_array.00102");

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(foo, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef p0 = LLVMGetParam(foo, 0);

    LLVMValueRef bound[] = { LLVMConstInt(LLVMInt32Type(), 0, 0) };
    LLVMValueRef v1a = LLVMBuildGEP(builder, p0, bound, 1, "v1a");
    LLVMValueRef v1 = LLVMBuildLoad(builder, v1a, "v1");

    // LLVMValueRef bound2[] = { LLVMConstInt(LLVMInt32Type(), 1, 0) };
    // LLVMValueRef v2a = LLVMBuildGEP(builder, p0, bound2, 2, "v2a");
    // LLVMValueRef v2 = LLVMBuildLoad(builder, v2a, "v2");

    param_types[0] = LLVMVoidType();
    ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 1, 0);
    LLVMValueRef bar = LLVMAddFunction(mod, "bar", ret_type);
    LLVMSetLinkage(bar, LLVMExternalLinkage);

    LLVMValueRef args[] = { LLVMConstInt(LLVMInt32Type(), 100, 0) };
    args[0] = NULL;
    LLVMBuildCall(builder, bar, args, 0, "");

    LLVMBuildRet(builder, v1);

    LLVMDumpModule(mod);
    return 0;
}
