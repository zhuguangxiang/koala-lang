#include <assert.h>
#include <inttypes.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitReader.h>
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
cc readbc.c -o readbc -std=gnu17 -I/usr/lib/llvm-14/include  -D_GNU_SOURCE \
-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -lLLVM-14
*/

int main(int argc, char *argv[])
{
    LLVMModuleRef mod = NULL;
    char *errormsg = NULL;
    LLVMMemoryBufferRef buf = NULL;
    LLVMCreateMemoryBufferWithContentsOfFile("./example.bc", &buf, &errormsg);
    LLVMParseBitcode(buf, &mod, &errormsg);
    LLVMDumpModule(mod);

    LLVMValueRef fn = LLVMGetNamedFunction(mod, "keep");
    // LLVMDeleteFunction(fn);
    LLVMSetLinkage(fn, LLVMInternalLinkage);

    LLVMTypeRef foo = LLVMGetTypeByName(mod, "struct.Foo");
    assert(foo);

    LLVMBasicBlockRef entry = LLVMGetFirstBasicBlock(fn);
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMValueRef inst = LLVMGetFirstInstruction(entry);
    LLVMPositionBuilderBefore(builder, inst);

    LLVMValueRef v = LLVMBuildLoad(builder, inst, "");
    LLVMValueRef z = LLVMBuildAlloca(builder, LLVMPointerType(foo, 0), "z");
    LLVMBuildStore(builder, z, v);

    LLVMDumpModule(mod);

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMPrintMessageAction, &error);

    return 0;
}
