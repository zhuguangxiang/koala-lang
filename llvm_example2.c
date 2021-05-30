/**
 * LLVM equivalent of:
 *
 * int sum(int a, int b) {
 *     return a + b;
 * }
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

/*
clang-format off
cc llvm_example2.c -o example -std=gnu11 -I/usr/lib/llvm-10/include  -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -lLLVM-10

 llc -filetype=obj -relocation-model=pic sum.bc
 ./sum
clang-format on
*/

int main(int argc, char *argv[])
{
    LLVMModuleRef mod = LLVMModuleCreateWithName("my_module");

    LLVMValueRef gval = LLVMAddGlobal(mod, LLVMInt32Type(), "gval");
    LLVMSetInitializer(gval, LLVMConstInt(LLVMInt32Type(), 0, 1));

    LLVMValueRef gval2 =
        LLVMAddGlobal(mod, LLVMPointerType(LLVMInt32Type(), 0), "gval2");

    LLVMSetInitializer(gval2,
                       LLVMConstNull(LLVMPointerType(LLVMInt32Type(), 0)));

    LLVMTypeRef str_struct_types[] = {
        LLVMInt8Type(),
        LLVMInt16Type(),
        LLVMPointerType(LLVMInt8Type(), 0),
    };

    LLVMTypeRef str_type =
        LLVMStructCreateNamed(LLVMGetGlobalContext(), "string");
    LLVMStructSetBody(str_type, str_struct_types, 3, 0);

    LLVMTypeRef param_types[] = { LLVMPointerType(str_type, 0) };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 1, 0);
    LLVMValueRef fn = LLVMAddFunction(mod, "println", ret_type);

    LLVMTypeRef array = LLVMStructCreateNamed(LLVMGetGlobalContext(), "Array");

    LLVMTypeRef elems[3];
    elems[0] = LLVMPointerType(LLVMInt8Type(), 0);
    elems[1] = LLVMInt32Type();
    elems[2] = LLVMInt32Type();
    LLVMStructSetBody(array, elems, 3, 0);

    LLVMTypeRef param_types2[] = { LLVMPointerType(array, 0) };
    LLVMTypeRef ret_type2 =
        LLVMFunctionType(LLVMVoidType(), param_types2, 1, 0);
    LLVMValueRef fn2 = LLVMAddFunction(mod, "println2", ret_type2);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(fn2, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef gv = LLVMBuildLoad(builder, gval, "gv");

    LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 100, 0), gval);

    /*
    LLVMValueRef gv2 = LLVMBuildArrayAlloca(
        builder, LLVMInt32Type(), LLVMConstInt(LLVMInt32Type(), 100, 0), "gv2");
    */

    LLVMValueRef gv2 = LLVMBuildArrayMalloc(
        builder, LLVMInt32Type(), LLVMConstInt(LLVMInt32Type(), 100, 0), "gv2");

    LLVMBuildStore(builder, gv2, gval2);

    LLVMBuildRetVoid(builder);

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    LLVMDumpModule(mod);

    if (LLVMWriteBitcodeToFile(mod, "example2.bc") != 0) {
        fprintf(stderr, "error writing bitcode to file, skipping\n");
    }

    return 0;
}
