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

#include "koala.h"
#include "opcode.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#define NEXT_OP() ({ \
  codes[index++]; \
})

#define NEXT_BYTE() ({ \
  codes[index++]; \
})

#define NEXT_2BYTES() ({ \
  uint8_t l = NEXT_BYTE(); \
  uint8_t h = NEXT_BYTE(); \
  ((h << 8) + l); \
})

static Object *exec_sum(void *ptr, Object *vargs)
{
  int (*code)(int, int) = ptr;
  Object *v1 = valist_get(vargs, 0);
  Object *v2 = valist_get(vargs, 1);
  int res = code(integer_asint(v1), integer_asint(v2));
  return integer_new(res);
}

/*
import "jit"

func sum3(a int, b int) int {
  return a + b + 300 + 400
}

jit.jit_sum(sum3, 20, 20)
*/
/*
  LOAD_CONST
  LOAD_2
  LOAD_1
  ADD
  ADD
  RETURN_VALUE
*/
static LLVMExecutionEngineRef engine;

static Object *jit(CodeObject *co, Object *vargs)
{
  if (engine != NULL) {
    printf("sum is exist.\n");
    void *ptr = (void *)LLVMGetFunctionAddress(engine, "sum");
    if (ptr != NULL) {
      Object *res = exec_sum(ptr, vargs);
      return res;
    } else {
      panic("cannot find sum in llvm");
      return NULL;
    }
  }

  LLVMModuleRef mod = LLVMModuleCreateWithName("test_jit_module");

  LLVMTypeRef param_types[] = { LLVMInt64Type(), LLVMInt64Type() };
  LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt64Type(), param_types, 2, 0);
  LLVMValueRef sum = LLVMAddFunction(mod, "sum", ret_type);

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");

  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  uint8_t *codes = co->codes;
  int argc = 2;
  int index = 0;
  uint8_t op;
  LLVMValueRef tmp;
  LLVMValueRef v1;
  LLVMValueRef v2;
  LLVMValueRef val;
  int oparg;
  Object *consts = co->consts;
  Object *x;
  VECTOR(stack);
  while (1) {
    if (index >= co->size)
      break;
    op = NEXT_OP();
    switch (op) {
    case OP_LOAD_CONST: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      val = LLVMConstInt(LLVMInt64Type(), integer_asint(x), 1);
      vector_push_back(&stack, val);
      break;
    }
    case OP_LOAD_1: {
      printf("OP_LOAD_1\n");
      if (argc >= 1) {
        val = LLVMGetParam(sum, 0);
        vector_push_back(&stack, val);
      }
      break;
    }
    case OP_LOAD_2: {
      printf("OP_LOAD_2\n");
      if (argc >= 2) {
        val = LLVMGetParam(sum, 1);
        vector_push_back(&stack, val);
      }
      break;
    }
    case OP_ADD: {
      printf("OP_ADD\n");
      v1 = vector_pop_back(&stack);
      v2 = vector_pop_back(&stack);
      val = LLVMBuildAdd(builder, v1, v2, "tmp");
      vector_push_back(&stack, val);
      break;
    }
    case OP_RETURN_VALUE: {
      printf("OP_RETURN_VALUE\n");
      val = vector_pop_back(&stack);
      LLVMBuildRet(builder, val);
      break;
    }
    default: {
      panic("invalid op: %d", op);
      break;
    }
    }
  }
  LLVMDisposeBuilder(builder);

  char *error = NULL;
  LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error);
  LLVMDumpModule(mod);

  LLVMLinkInMCJIT();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMCreateExecutionEngineForModule(&engine, mod, &error);
  void *ptr = (void *)LLVMGetFunctionAddress(engine, "sum");
  Object *res = exec_sum(ptr, vargs);
  //LLVMDisposeExecutionEngine(engine);
  return res;
}

Object *jit_sum(Object *self, Object *args)
{
  Object *meth = tuple_get(args, 0);
  CodeObject *co = (CodeObject *)method_getcode(meth);
  Object *vargs = tuple_get(args, 1);
  Object *res = jit(co, vargs);
  OB_DECREF(meth);
  OB_DECREF(vargs);
  return res;
}

static MethodDef jit_methods[] = {
  {"jit_sum", "A...i", "i", jit_sum},
  {NULL}
};

void init_jit_module(void)
{
  Object *m = module_new("jit");
  module_add_type(m, &any_type);
  module_add_funcdefs(m, jit_methods);
  module_install("jit", m);
  OB_DECREF(m);
}

void fini_jit_module(void)
{
  module_uninstall("jit");
}
