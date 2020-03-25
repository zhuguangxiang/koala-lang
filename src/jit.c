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
#include "jit.h"
#include "jit_llvm.h"

static Object *jit_func_wrapper(Object *self, Object *args, void *jitptr)
{
  jit_func_t *fninfo = jitptr;
  return jit_ffi_call(fninfo, args);
}

#define NEXT_OP() ({ \
  codes[index++]; \
})

#define NEXT_BYTE() ({ \
  codes[index++]; \
})

#define NEXT_2BYTES() ({ \
  int16_t v = *(int16_t *)(codes + index); \
  index += 2; \
  v; \
})

#define POP()   vector_pop_back(&ctx.stack)
#define PUSH(v) vector_push_back(&ctx.stack, v)

#define GETLOCAL(i) ({ \
  JitValue *var = vector_get(&ctx.locals, i); \
  var->llvalue; \
})

#define SETLOCAL(i, v) ({\
  JitValue *var = vector_get(&ctx.locals, i); \
  jit_store_value(&ctx, var, (v)); \
})

typedef struct translate {
  void *engine;
  void *mod;
  CodeObject *co;
  Object *consts;
  Vector *locinfo;
  Vector blocks;
  void *mcptr;
  Vector stack;
  Vector locvars;
} Translate;

typedef struct basicblock {
  uint8_t *codes;
  int size;
  Translate *trans;
} BasicBlock;

static void split_blocks(Translate *trans)
{
  CodeObject *co = trans->co;
  uint8_t *codes = co->codes;
  int size = co->size;
  uint8_t op;
  int oparg;
  int index = 0;
  BasicBlock *blk;
  uint8_t *start = codes;
  uint8_t *end = codes;
  while (index < size) {
    op = NEXT_OP();
    switch (op) {
    case OP_JMP_FALSE: {
      printf("%-16s2\n", "JMP_FALSE");
      oparg = NEXT_2BYTES();
      end = codes + index;
      printf("\x1b[1;35m%-14s\x1b[0m\t%ld-%ld\n", "Block", start - codes, end - codes);
      //blk = new_basicblock(start, end);
      //vector_push_back(&trans->blocks, blk);
      start = end;
      break;
    }
    case OP_JMP: {
      printf("%-16s0\n", "JMP");
      oparg = NEXT_2BYTES();
      break;
    }
    default: {
      int argc = opcode_argc(op);
      printf("%-16s%d\n", opcode_str(op), argc);
      index += argc;
      break;
    }
    }
  }
}

static int jit_support(CodeObject *co)
{
  return 1;
}

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

func sum(a int, b int) int {
  return a - b
}

jit.go(sum)
*/
/*
  LOAD_CONST
  LOAD_2
  LOAD_1
  ADD
  ADD
  RETURN_VALUE
*/
/*
  func locvar(a int, b int) int {
    v := 200
    return a - b + v
  }
  import "jit"
  jit.go(locvar, 10, 20)
 */
/*
  g := 200
  func gblvar(a int, b int) int {
    g = 300
    v := a - b + g
    g = 500
    return v
  }
  import "jit"
  jit.go(gblvar, 10, 20)

  func gblvar2(a int, b int) int {
    return a - b + g
  }
  jit.go(gblvar2, 10, 20)
 */

/*
func branch(a int, b int) int {
  res := 0
  if a > 100 {
    return b + 100
  } else {
    res = b - 100
  }
  return res
}

func branch2(a int, b int) int {
  res := 0
  if a > 100 {
    return b + 100
  }
  return res
}

func branch3(a int, b int) int {
  res := 0
  if a > 100 {
    if (a > 200) {
      return b + 100
    } else {
      if a < 150 {
        return b + 200
      }
    }
  }
  return res
}

func branch4(a int, b int) int {
  res := 0
  if a > 100 {
    return b + 100
  } else if a < 50 {
    return b + 50
  } else {
    return b + 1
  }
  return res
}

import "jit"
jit.go(branch, 10, 20)
*/

/*
func fib(n int) int {
  if n < 3 {
    return 1
  }
  return fib(n - 1) + fib(n - 2)
}
import "jit"
jit.go(fib, 40, 0)
 */

static void translate_basicblock()
{

}

static JitFunction *translate(CodeObject *co)
{
  uint8_t *codes = co->codes;
  Object *consts = co->consts;
  int size = co->size;
  int index = 0;
  uint8_t op;
  int oparg;
  Object *x;
  Vector *vec;

  JitContext ctx;
  JitFunction *fn = jit_function(co);
  jit_context_init(&ctx, fn);

  JitValue *var;
  LLVMValueRef val, lhs, rhs, v1, v2;
  LLVMBuilderRef builder = ctx.builder;

  while (index < size) {
    op = NEXT_OP();
    switch (op) {
    case OP_LOAD_CONST: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      val = jit_const(x);
      PUSH(val);
      OB_DECREF(x);
      break;
    }
    case OP_LOAD: {
      oparg = NEXT_BYTE();
      val = GETLOCAL(oparg);
      PUSH(val);
      break;
    }
    case OP_LOAD_0: {
      val = GETLOCAL(0);
      PUSH(val);
      break;
    }
    case OP_LOAD_1: {
      val = GETLOCAL(1);
      PUSH(val);
      break;
    }
    case OP_LOAD_2: {
      val = GETLOCAL(2);
      PUSH(val);
      break;
    }
    case OP_LOAD_3: {
      val = GETLOCAL(3);
      PUSH(val);
      break;
    }
    case OP_STORE: {
      oparg = NEXT_BYTE();
      val = POP();
      SETLOCAL(oparg, val);
      break;
    }
    case OP_STORE_0: {
      val = POP();
      SETLOCAL(0, val);
      break;
    }
    case OP_STORE_1: {
      val = POP();
      SETLOCAL(1, val);
      break;
    }
    case OP_STORE_2: {
      val = POP();
      SETLOCAL(2, val);
      break;
    }
    case OP_STORE_3: {
      val = POP();
      SETLOCAL(3, val);
      break;
    }
    case OP_RETURN_VALUE: {
      val = POP();
      LLVMBuildRet(builder, val);
      break;
    }
    case OP_RETURN: {
      LLVMBuildRetVoid(builder);
      break;
    }
    case OP_ADD: {
      lhs = POP();
      rhs = POP();
      val = LLVMBuildAdd(builder, lhs, rhs, "tmp");
      PUSH(val);
      break;
    }
    case OP_NEW_TUPLE: {
      oparg = NEXT_2BYTES();
      if (oparg > 0) {
        vec = vector_new();
        for (int i = 0; i < oparg; ++i) {
          v1 = POP();
          vector_push_back(vec, v1);
        }
      } else {
        vec = NULL;
      }
      val = jit_OP_NEW_TUPLE(&ctx, vec);
      vector_free(vec);
      PUSH(val);
      break;
    }
    case OP_NEW: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        v1 = NULL;
      } else {
        v1 = POP();
      }
      TypeDesc *desc = descob_getdesc(x);
      if (desc_isbase(desc)) {
        val = v1;
      } else {
        val = jit_OP_NEW(&ctx, co, desc);
      }
      OB_DECREF(x);
      PUSH(val);
      break;
    }
    default: {
      panic("invalid op: %d", op);
      break;
    }
    }
  }

  jit_verify_ir();
  jit_context_fini(&ctx);
  return fn;
}

/*
static void *translate(CodeObject *co)
{
  LLVMModuleRef mod = llvm_module();
  LLVMValueRef func = llvm_function(co);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
  int argc = LLVMCountParams(func);
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  uint8_t *codes = co->codes;
  int index = 0;
  uint8_t op;
  LLVMValueRef tmp;
  LLVMValueRef v1;
  LLVMValueRef v2;
  LLVMValueRef val;
  int oparg;
  Object *consts = co->consts;
  Object *x, *y, *z;
  VECTOR(stack);
  VECTOR(locvars);
  Object *curmod;
  LLVMBasicBlockRef cond_true = NULL;
  LLVMBasicBlockRef cond_false = NULL;
  LLVMBasicBlockRef end = NULL;
  int cond_cont = 0;
  LLVMValueRef zero = LLVMConstInt(LLVMInt64Type(), 0, 1);
  while (1) {
    if (index >= co->size)
      break;
    op = NEXT_OP();
    switch (op) {
    case OP_LOAD_CONST: {
      printf("OP_LOAD_CONST\n");
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      val = LLVMConstInt(LLVMInt64Type(), integer_asint(x), 1);
      vector_push_back(&stack, val);
      OB_DECREF(x);
      break;
    }
    case OP_LOAD_1: {
      printf("OP_LOAD_1\n");
      if (argc >= 1) {
        val = LLVMGetParam(func, 0);
        vector_push_back(&stack, val);
      }
      break;
    }
    case OP_LOAD_2: {
      printf("OP_LOAD_2\n");
      if (argc >= 2) {
        val = LLVMGetParam(func, 1);
        vector_push_back(&stack, val);
      }
      break;
    }
    case OP_LOAD_3: {
      printf("OP_LOAD_3\n");
      if (argc >= 3) {
        // func parameter
        val = LLVMGetParam(func, 2);
        vector_push_back(&stack, val);
      } else {
        // local variable
        v1 = vector_get(&locvars, 3);
        vector_push_back(&stack, v1);
      }
      break;
    }
    case OP_STORE_3: {
      printf("OP_STORE_3\n");
      if (argc >= 3) {

      } else {
        char name[32] = {0};
        int locindex = 3;
        snprintf(name, 32, "loc_%d", locindex);
        while (vector_size(&locvars) < locindex) {
          vector_push_back(&locvars, NULL);
        }
        v1 = vector_get(&locvars, locindex);
        if (v1 == NULL) {
          printf("new locvar: %s\n", name);
          v1 = LLVMBuildAlloca(builder, LLVMInt64Type(), name);
          v2 = vector_pop_back(&stack);
          vector_set(&locvars, 3, v2);
          LLVMBuildStore(builder, v2, v1);
        } else {
          printf("store locvar: %s\n", name);
          v2 = vector_pop_back(&stack);
          vector_set(&locvars, 3, v2);
        }
      }
      break;
    }
    case OP_ADD: {
      printf("OP_ADD\n");
      v1 = vector_pop_back(&stack);
      v2 = vector_pop_back(&stack);
      expect(v2 != NULL);
      val = LLVMBuildAdd(builder, v1, v2, "tmp");
      vector_push_back(&stack, val);
      break;
    }
    case OP_SUB: {
      printf("OP_SUB\n");
      v1 = vector_pop_back(&stack);
      v2 = vector_pop_back(&stack);
      val = LLVMBuildSub(builder, v1, v2, "tmp");
      vector_push_back(&stack, val);
      break;
    }
    case OP_RETURN_VALUE: {
      printf("OP_RETURN_VALUE\n");
      val = vector_pop_back(&stack);
      LLVMBuildRet(builder, val);
      break;
    }
    case OP_RETURN: {
      printf("OP_RETURN\n");
      //LLVMBuildRetVoid(builder);
      break;
    }
    case OP_LOAD_GLOBAL: {
      printf("OP_LOAD_GLOBAL\n");
      curmod = co->module;
      vector_push_back(&stack, curmod);
      break;
    }
    case OP_GET_VALUE: {
      printf("OP_GET_VALUE\n");
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = vector_pop_back(&stack);
      printf("get global variable:%s\n", string_asstr(x));
      v1 = LLVMGetNamedGlobal(mod, string_asstr(x));
      if (v1 == NULL) {
        printf("global %s is not set\n", string_asstr(x));
        v1 = LLVMAddGlobal(mod, LLVMInt64Type(), string_asstr(x));
        z = object_getvalue(y, string_asstr(x), co->type);
        val = LLVMConstInt(LLVMInt64Type(), integer_asint(z), 1);
        LLVMSetInitializer(v1, val);
      } else {
        val = LLVMGetInitializer(v1);
      }
      vector_push_back(&stack, val);
      OB_DECREF(x);
      OB_DECREF(z);
      break;
    }
    case OP_SET_VALUE: {
      printf("OP_SET_VALUE\n");
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = vector_pop_back(&stack);
      printf("set global variable:%s\n", string_asstr(x));
      v1 = LLVMGetNamedGlobal(mod, string_asstr(x));
      if (v1 == NULL) {
        printf("global %s is not set\n", string_asstr(x));
        v1 = LLVMAddGlobal(mod, LLVMInt64Type(), string_asstr(x));
      }
      val = vector_pop_back(&stack);
      LLVMSetInitializer(v1, val);
      OB_DECREF(x);
      break;
    }
    case OP_GT: {
      printf("OP_GT\n");
      v1 = vector_pop_back(&stack);
      v2 = vector_pop_back(&stack);
      val = LLVMBuildICmp(builder, LLVMIntSGT, v1, v2, "tmp");
      vector_push_back(&stack, val);
      break;
    }
    case OP_LT: {
      printf("OP_LT\n");
      v1 = vector_pop_back(&stack);
      v2 = vector_pop_back(&stack);
      val = LLVMBuildICmp(builder, LLVMIntSLT, v1, v2, "tmp");
      vector_push_back(&stack, val);
      break;
    }
    case OP_JMP_FALSE: {
      printf("OP_JMP_FALSE\n");
      oparg = (int16_t)NEXT_2BYTES();
      val = vector_pop_back(&stack);
      cond_true = LLVMAppendBasicBlock(func, "cond_true");
      cond_false = LLVMAppendBasicBlock(func, "cond_false");
      //end = LLVMAppendBasicBlock(func, "end");
      LLVMBuildCondBr(builder, val, cond_true, cond_false);
      if (oparg > 0) {
        LLVMPositionBuilderAtEnd(builder, cond_true);
        cond_cont = oparg;
      } else {
        LLVMPositionBuilderAtEnd(builder, cond_false);
      }
      break;
    }
    case OP_JMP: {
      printf("OP_JMP\n");
      oparg = (int16_t)NEXT_2BYTES();
      LLVMPositionBuilderAtEnd(builder, cond_false);
      cond_cont = oparg;

      //if (oparg <= 0) {
      //  LLVMBuildBr(builder, end);
      //  LLVMPositionBuilderAtEnd(builder, end);
      //}

      break;
    }
    case OP_CALL: {
      printf("OP_CALL\n");
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      oparg = NEXT_BYTE();
      vector_pop_back(&stack);
      v1 = vector_pop_back(&stack);
      v2 = vector_pop_back(&stack);
      printf("call:%s\n", string_asstr(x));
      LLVMValueRef call_args[2] = {v1, v2};

      int argc;
      LLVMValueRef *args;
      LLVMValueRef fn = NULL;
      LLVMFindFunction(llvm_engine(), string_asstr(x), &fn);
      if (fn == NULL) {
        LLVMValueRef kfunc_args[4] = {};
        args = kfunc_args;
        argc = 4;
        fn = jit_kfunc_entry();
      } else {
        LLVMValueRef llvm_args[oparg];

        args = llvm_args;
        argc = oparg
      }
      val = LLVMBuildCall(builder, fn, args, argc, string_asstr(x));
      vector_push_back(&stack, val);
      OB_DECREF(x);
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
  LLVMVerifyModule(mod, LLVMPrintMessageAction, &error);
  LLVMDisposeMessage(error);
  LLVMDumpModule(mod);

  void *v = LLVMRecompileAndRelinkFunction(llvm_engine(), kfunc);
  printf("%p\n", v);
  void *ptr = (void *)LLVMGetFunctionAddress(llvm_engine(), llvm_function_name(co));

  vector_fini(&stack);
  vector_fini(&locvars);
  return ptr;
}
*/

/*
  func go(f any) Option<Method>
*/
Object *jit_go(Object *self, Object *args)
{
  Object *ob = args;
  if (!method_check(ob)) {
    warn("[KOALA-JIT]: '%s' is not a Fuction.", OB_TYPE_NAME(ob));
    return option_none();
  }

  MethodObject *meth = (MethodObject *)ob;
  if (!module_check(meth->owner)) {
    warn("[KOALA-JIT]: only Function supported.");
    return option_none();
  }

  if (meth->kind == JITFUNC_KIND) {
    debug("[KOALA-JIT]: already jit function");
    return option_some(ob);
  }

  if (meth->kind != KFUNC_KIND) {
    warn("[KOALA-JIT]: only Koala Function supported.");
    return option_none();
  }

  CodeObject *co = meth->ptr;
  if (!jit_support(co)) {
    return option_none();
  }

  JitFunction *fn = translate(co);
  void *mcptr = jit_emit_code(fn);
  if (mcptr == NULL) {
    warn("[KOALA-JIT]: '%s' is translated failed.", meth->name);
    return option_none();
  }

  jit_func_t *fninfo = jit_get_func(mcptr, meth->desc);
  method_update_jit(meth, jit_func_wrapper, fninfo);
  jit_free_function(fn);
  return option_some(ob);
}

void jit_initialize(void)
{
  init_jit_llvm();
}

void jit_finalize(void)
{
  fini_jit_llvm();
  fini_jit_ffi();
}
