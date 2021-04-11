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
 cc llvm_example.c -o sum -std=gnu17 -I/usr/lib/llvm-11/include  -D_GNU_SOURCE \
 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -lLLVM-11

 llc -filetype=obj -relocation-model=pic sum.bc
 ./sum
*/

struct builder_if_state {
    LLVMValueRef condition;
    LLVMBasicBlockRef entry_block;
    LLVMBasicBlockRef true_block;
    LLVMBasicBlockRef false_block;
    LLVMBasicBlockRef merge_block;
};

LLVMBasicBlockRef build_insert_new_block(LLVMBuilderRef builder, char *name)
{
    LLVMBasicBlockRef current_block;
    LLVMBasicBlockRef next_block;
    LLVMBasicBlockRef new_block;

    current_block = LLVMGetInsertBlock(builder);

    next_block = LLVMGetNextBasicBlock(current_block);
    if (next_block) {
        next_block = LLVMInsertBasicBlock(next_block, name);
    } else {
        LLVMValueRef func = LLVMGetBasicBlockParent(current_block);
        new_block = LLVMAppendBasicBlock(func, name);
    }
    return new_block;
}

void build_if(struct builder_if_state *ifthen, LLVMBuilderRef builder,
              LLVMValueRef condition)
{
    LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
    ifthen->condition = condition;
    ifthen->entry_block = block;

    ifthen->merge_block = build_insert_new_block(builder, "endif-block");
    ifthen->true_block =
        LLVMInsertBasicBlock(ifthen->merge_block, "if-true-block");

    LLVMPositionBuilderAtEnd(builder, ifthen->true_block);
}

void build_else(struct builder_if_state *ifthen, LLVMBuilderRef builder)
{
    LLVMBuildBr(builder, ifthen->merge_block);

    ifthen->false_block =
        LLVMInsertBasicBlock(ifthen->merge_block, "if-false-block");

    LLVMPositionBuilderAtEnd(builder, ifthen->false_block);
}

void builde_endif(struct builder_if_state *ifthen, LLVMBuilderRef builder)
{
    LLVMBuildBr(builder, ifthen->merge_block);

    LLVMPositionBuilderAtEnd(builder, ifthen->entry_block);
    if (ifthen->false_block) {
        LLVMBuildCondBr(builder, ifthen->condition, ifthen->true_block,
                        ifthen->false_block);
    } else {
        LLVMBuildCondBr(builder, ifthen->condition, ifthen->true_block,
                        ifthen->merge_block);
    }

    LLVMPositionBuilderAtEnd(builder, ifthen->merge_block);
}

static int val;

/*
unsigned gcd(unsigned x, unsigned y) {
  if(x == y) {
    return x;
  } else if(x < y) {
    return gcd(x, y - x);
  } else {
    return gcd(x - y, y);
  }
}
 */
void create_if_block(LLVMBuilderRef builder, LLVMBasicBlockRef up,
                     LLVMValueRef cond, LLVMBasicBlockRef body,
                     LLVMBasicBlockRef else_body)
{
    LLVMPositionBuilderAtEnd(builder, up);
    LLVMBuildCondBr(builder, cond, body, else_body);
}

LLVMValueRef gcd;

void gen_gcd(LLVMModuleRef mod)
{
    struct builder_if_state ifthen = { NULL };
    struct builder_if_state ifthen2 = { NULL };

    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
    gcd = LLVMAddFunction(mod, "gcd", ret_type);

    LLVMValueRef lhs = LLVMGetParam(gcd, 0);
    LLVMValueRef rhs = LLVMGetParam(gcd, 1);

    LLVMSetValueName(lhs, "x");
    LLVMSetValueName(rhs, "y");

    // function entry
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(gcd, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);
    // condition
    LLVMValueRef If = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "cond");

    // three blocks
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(gcd, "if_then");
    LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(gcd, "if_else");
    // LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(gcd, "if_end");

    // jump in entry block
    LLVMBuildCondBr(builder, If, then_block, else_block);

    // edit then block
    LLVMPositionBuilderAtEnd(builder, then_block);
    LLVMValueRef z = LLVMBuildAlloca(builder, LLVMInt32Type(), "z");
    // LLVMSetAlignment(z, 8);
    int align = LLVMGetAlignment(z);
    printf("alignment of alloca int32:%d\n", align);
    LLVMBuildStore(builder, LLVMBuildAdd(builder, lhs, rhs, ""), z);
    LLVMBuildRet(builder, lhs);

    // LLVMStructCreateNamed(LLVMGetGlobalContext(), "Person");

    //  check instructions in then block
    LLVMValueRef inst = LLVMGetFirstInstruction(then_block);
    while (inst) {
        LLVMOpcode code = LLVMGetInstructionOpcode(inst);
        inst = LLVMGetNextInstruction(inst);
        if (inst && code == LLVMRet) {
            printf("error: return must be last one.\n");
            abort();
        }
    }
    //  LLVMBuildBr(builder, end_block);

    // edit else block
    LLVMPositionBuilderAtEnd(builder, else_block);

    // condition in else block
    If = LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "cond2");
    // append
    then_block = LLVMAppendBasicBlock(gcd, "if_then2");
    LLVMMoveBasicBlockAfter(then_block, else_block);
    else_block = LLVMAppendBasicBlock(gcd, "if_else2");
    LLVMMoveBasicBlockAfter(else_block, then_block);
    // jump in else block
    LLVMBuildCondBr(builder, If, then_block, else_block);

    // edit then_block in up else_block
    LLVMPositionBuilderAtEnd(builder, then_block);
    LLVMBuildRet(builder, LLVMBuildAdd(builder, lhs, rhs, "ret2"));
    // LLVMBuildBr(builder, end_block);

    // edit else_block in up else_blck
    LLVMPositionBuilderAtEnd(builder, else_block);
    LLVMBuildRet(builder, LLVMBuildSub(builder, lhs, rhs, "ret3"));
    // LLVMBuildBr(builder, end_block);

    LLVMDisposeBuilder(builder);
}

typedef struct string {
    char len;
    short hoo;
    char *str;
} kl_string_t;

kl_string_t one_str = { 4, 10, "ah" };

kl_string_t *__kl_string_new(char *s)
{
    one_str.len = strlen(s);
    one_str.str = s;
    return &one_str;
}

void __println(kl_string_t *s)
{
    printf("%s,%d\n", s->str, s->len);
}

LLVMValueRef print_func;
LLVMValueRef str_new_func;

void gen_print_hello(LLVMModuleRef mod, LLVMExecutionEngineRef engine)
{
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
    print_func = LLVMAddFunction(mod, "println", ret_type);
    // LLVMSetLinkage(print_func, LLVMExternalLinkage);

    LLVMTypeRef param_types2[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef ret_type2 =
        LLVMFunctionType(LLVMPointerType(str_type, 0), param_types2, 1, 0);
    str_new_func = LLVMAddFunction(mod, "kl_string_new", ret_type2);
    // LLVMSetLinkage(str_new_func, LLVMExternalLinkage);

    LLVMTypeRef arr_type =
        LLVMStructCreateNamed(LLVMGetGlobalContext(), "array");
    LLVMTypeRef param_types3[] = { LLVMPointerType(str_type, 0) };
    ret_type = LLVMFunctionType(LLVMVoidType(), param_types3, 1, 0);
    LLVMValueRef main_func = LLVMAddFunction(mod, "main2", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);
    // LLVMValueRef v = LLVMBuildGlobalStringPtr(builder, "hello,world", "str");
    // LLVMConstString("hello,world", 11, 1),
    LLVMValueRef v = LLVMGetParam(main_func, 0);

    /*
    LLVMValueRef gep[3];
    gep[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
    gep[1] = LLVMConstInt(LLVMInt32Type(), 2, 0);
    LLVMValueRef v2 = LLVMBuildGEP(builder, v, gep, 2, "v2");
    LLVMValueRef v3 = LLVMBuildLoad(builder, v2, "foo");
    */
    LLVMValueRef v2 = LLVMBuildStructGEP(builder, v, 2, "v2");
    LLVMValueRef v3 = LLVMBuildLoad(builder, v2, "foo");

    LLVMValueRef args[] = {
        v3,
        NULL,
    };
    LLVMValueRef ret = LLVMBuildCall(builder, str_new_func, args, 1, "hello");

    LLVMValueRef args2[] = {
        ret,
        NULL,
    };
    LLVMBuildCall(builder, print_func, args2, 1, "");

    LLVMValueRef args3[] = {
        LLVMConstInt(LLVMInt32Type(), 100, 0),
        LLVMConstInt(LLVMInt32Type(), 200, 0),
        NULL,
    };

    LLVMBuildCall(builder, gcd, args3, 2, "NULL");

    LLVMBuildRetVoid(builder);

    // LLVMDisposeBuilder(builder);
}

int bar();

void main(int argc, char *argv[])
{
    LLVMModuleRef mod = LLVMModuleCreateWithName("my_module");

    LLVMValueRef gval = LLVMAddGlobal(mod, LLVMInt32Type(), "gval");
    LLVMSetSection(gval, "init_globals");
    // LLVMSetGlobalConstant(gval, 1);
    // LLVMSetLinkage(gval, LLVMPrivateLinkage);
    // LLVMSetLinkage(gval, LLVMAvailableExternallyLinkage);
    LLVMSetInitializer(gval, LLVMConstInt(LLVMInt32Type(), 0, 1));

    // LLVMSetVisibility(gval, LLVMProtectedVisibility);

    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };

    LLVMTypeRef ret_type2 =
        LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    LLVMValueRef bar_func = LLVMAddFunction(mod, "bar", ret_type2);

    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
    LLVMValueRef sum = LLVMAddFunction(mod, "sum", ret_type);
    LLVMSetSection(sum, "sum_funcs");
    LLVMValueRef a = LLVMGetParam(sum, 0);
    // LLVMSetLinkage(sum, LLVMInternalLinkage);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");

    // call external function
    // func bar() int

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    // LLVMBuildCall(builder, bar_func, NULL, 0, "bar");

    LLVMValueRef tmp = LLVMBuildAdd(builder, LLVMGetParam(sum, 0),
                                    LLVMGetParam(sum, 1), "tmp");
    // LLVMBuildBr(builder, entry);
    LLVMBuildRet(builder, tmp);

    LLVMDisposeBuilder(builder);
    LLVMDumpModule(mod);
    LLVMSetValueName(a, "a");
    LLVMDumpModule(mod);

    gen_gcd(mod);

    LLVMTypeRef st = LLVMStructCreateNamed(LLVMGetGlobalContext(), "Foo");
    LLVMTypeRef elems[2];
    elems[0] = LLVMInt32Type();
    elems[1] = LLVMInt32Type();
    LLVMStructSetBody(st, elems, 2, 0);

    LLVMValueRef args[2];
    args[0] = LLVMConstInt(LLVMInt32Type(), 100, 0);
    args[1] = LLVMConstInt(LLVMInt32Type(), 200, 0);
    LLVMValueRef v = LLVMConstNamedStruct(st, args, 2);

    // LLVMValueRef gv = LLVMAddGlobal(mod, st, "foo");
    // LLVMSetInitializer(gv, v);
    // LLVMSetGlobalConstant(gv, 1);
    // LLVMSetLinkage(gv, LLVMPrivateLinkage);

    // LLVMValueRef gv2 = LLVMAddGlobal(mod, st, "gFoo");
    // LLVMSetInitializer(gv, v);
    // LLVMSetGlobalConstant(gv2, 1);
    // LLVMSetLinkage(gv2, LLVMPrivateLinkage);

    gen_print_hello(mod, NULL);

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMPrintMessageAction, &error);
    LLVMDisposeMessage(error);

    LLVMExecutionEngineRef engine;
    error = NULL;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) != 0) {
        fprintf(stderr, "failed to create execution engine\n");
        abort();
    }
    if (error) {
        fprintf(stderr, "error: %s\n", error);
        LLVMDisposeMessage(error);
        exit(EXIT_FAILURE);
    }

    LLVMSetLinkage(bar_func, LLVMExternalLinkage);
    // LLVMAddGlobalMapping(engine, bar_func, bar);
    LLVMAddGlobalMapping(engine, print_func, __println);
    LLVMAddGlobalMapping(engine, str_new_func, __kl_string_new);

    LLVMDumpModule(mod);
#if 0
  LLVMPassManagerRef pass = LLVMCreatePassManager();
  //  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass);
  LLVMAddConstantPropagationPass(pass);
  LLVMAddInstructionCombiningPass(pass);
  LLVMAddPromoteMemoryToRegisterPass(pass);
  LLVMAddGVNPass(pass);
  LLVMAddCFGSimplificationPass(pass);
  LLVMAddDCEPass(pass);
  LLVMRunPassManager(pass, mod);

  LLVMDumpModule(mod);
#endif
    /*
      if (argc < 3) {
        fprintf(stderr, "usage: %s x y\n", argv[0]);
        // exit(EXIT_FAILURE);
      }
      long long x = strtoll(argv[1], NULL, 10);
      long long y = strtoll(argv[2], NULL, 10);

      LLVMGenericValueRef args[] = { LLVMCreateGenericValueOfInt(
                                       LLVMInt32Type(), x, 0),
        LLVMCreateGenericValueOfInt(LLVMInt32Type(), y, 0) };
      int (*sum_func)(int, int) =
        (int (*)(int, int))LLVMGetFunctionAddress(engine, "sum");
      printf("%d\n", sum_func(x, y));

    long long x = 100;
    long long y = 200;
    int (*sum_func)(int, int) = (int (*)(int,
    int))LLVMGetFunctionAddress(engine, "sum");
  */

    void (*main_func)(kl_string_t *) =
        (void (*)(kl_string_t *))LLVMGetFunctionAddress(engine, "main2");
    kl_string_t s = { 3, 10, "foo" };
    main_func(&s);

    // Write out bitcode to file
    if (LLVMWriteBitcodeToFile(mod, "sum.bc") != 0) {
        fprintf(stderr, "error writing bitcode to file, skipping\n");
    }

    // LLVMDisposeExecutionEngine(engine);
}
