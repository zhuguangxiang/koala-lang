
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
*/

int main(int argc, char *argv[])
{
    LLVMModuleRef mod = LLVMModuleCreateWithName("test_gc");

    LLVMTypeRef param_types[] = { LLVMVoidType() };

    LLVMTypeRef ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 0, 0);
    LLVMValueRef foo = LLVMAddFunction(mod, "foo", ret_type);
    LLVMSetGC(foo, "statepoint-example");
    LLVMSetSection(foo, ".init_array.00102");

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(foo, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    // LLVMValueRef v =
    //     LLVMBuildAlloca(builder, LLVMPointerType(LLVMInt8Type(), 1), "v");

    param_types[0] = LLVMInt32Type();
    ret_type =
        LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 1), param_types, 1, 0);
    LLVMValueRef mfn = LLVMAddFunction(mod, "malloc", ret_type);
    LLVMSetLinkage(mfn, LLVMExternalLinkage);

    LLVMValueRef args[] = { LLVMConstInt(LLVMInt32Type(), 100, 0) };
    LLVMValueRef v2 = LLVMBuildCall(builder, mfn, args, 1, "v2");
    // LLVMSetTailCall(v2, 1);
    LLVMBuildStore(builder, LLVMConstInt(LLVMInt8Type(), 10, 0), v2);
    // LLVMBuildStore(builder, v2, v);

    param_types[0] = LLVMVoidType();
    ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 1, 0);
    LLVMValueRef bar = LLVMAddFunction(mod, "bar", ret_type);
    LLVMSetLinkage(bar, LLVMExternalLinkage);

    args[0] = NULL;
    LLVMBuildCall(builder, bar, args, 0, "");

    LLVMBuildStore(builder, LLVMConstInt(LLVMInt8Type(), 20, 0), v2);

    LLVMBuildRetVoid(builder);

    LLVMDumpModule(mod);
    return 0;
}
