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

#include "jit_llvm.h"

JITType JITIntType    = {NULL, 1, "int"  };
JITType JITByteType   = {NULL, 1, "byte" };
JITType JITFloatType  = {NULL, 1, "float"};
JITType JITBoolType   = {NULL, 1, "bool" };
JITType JITCharType   = {NULL, 1, "char" };
JITType JITStrType    = {NULL, 1, "str"  };
JITType JITKlassType  = {NULL, 0, "klass"};

static void init_types(void)
{
  JITIntType.lltype   = LLVMInt64Type();
  JITByteType.lltype  = LLVMInt8Type();
  JITFloatType.lltype = LLVMDoubleType();
  JITBoolType.lltype  = LLVMInt8Type();
  JITCharType.lltype  = LLVMInt32Type();
  JITStrType.lltype   = LLVMStringType();
  JITKlassType.lltype = LLVMVoidPtrType();
}

JITType jit_type(TypeDesc *desc)
{
  if (!desc_isbase(desc)) {
    return JITKlassType;
  }

  JITType type = {NULL, 1, NULL};
  int kind = desc->base;
  switch (kind) {
  case BASE_INT:
    type = JITIntType;
    break;
  case BASE_BYTE:
    type = JITByteType;
    break;
  case BASE_FLOAT:
    type = JITFloatType;
    break;
  case BASE_BOOL:
    type = JITBoolType;
    break;
  case BASE_STR:
    type = JITStrType;
    break;
  case BASE_CHAR:
    type = JITCharType;
    break;
  case BASE_ANY:
    type = JITKlassType;
    break;
  default:
    panic("why go here?");
    break;
  }
  return type;
}

static Object *val_to_obj(void *val, TypeDesc *desc)
{
  if (!desc_isbase(desc)) {
    // koala's object
    return val;
  }
/*
  // koala's type is base type
  struct base_mapping *m = base_mappings;
  while (m->type != NULL) {
    if (m->kind == desc->base) {
      return m->object(rval);
    }
    ++m;
  }
*/
}

static void *obj_to_val()
{
  return NULL;
}

static void *kfunc_call(void *self, void *ob, void *args[], int32_t argc)
{
  Object *value;
  if (argc <= 0) {
    value = NULL;
  } else if (argc == 1) {

  } else {
    value = tuple_new(argc);
  }

  return NULL;
}

static void kobj_decref(void *self)
{

}

static void kobj_incref(void *self)
{

}

static void jit_initargs(void *args[], int32_t argc)
{

}

static void jit_clearstk(void *args[], int32_t argc)
{

}

static JITState gstate;

static LLVMModuleRef llvm_module(void)
{
  if (gstate.mod == NULL) {
    gstate.mod = LLVMModuleCreateWithName("__koala_jitter__");
  }
  return gstate.mod;
}

static LLVMExecutionEngineRef llvm_engine(void)
{
  if (gstate.engine == NULL) {
    char *error;
    LLVMExecutionEngineRef engine;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMCreateExecutionEngineForModule(&engine, llvm_module(), &error);
    gstate.engine = engine;
  }
  return gstate.engine;
}

static char *function_name(CodeObject *co)
{
  char *name;
  if (co->longname != NULL) {
    name = co->longname;
  } else {
    Object *mo = co->module;
    STRBUF(sbuf);
    strbuf_append(&sbuf, MODULE_PATH(mo));
    strbuf_append(&sbuf, "::");
    if (co->type != NULL) {
      strbuf_append(&sbuf, co->type->name);
      strbuf_append(&sbuf, "::");
    }
    strbuf_append(&sbuf, co->name);
    name = atom(strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
    co->longname = name;
  }
  return name;
}

void *jit_emit_code(JITFunction *func)
{
  char *name = function_name(func->co);
  return (void *)LLVMGetFunctionAddress(llvm_engine(), name);
}

static void init_wheels(void)
{
  LLVMTypeRef proto;
  LLVMTypeRef params[4];
  params[0] = LLVMVoidPtrType();
  params[1] = LLVMVoidPtrType();
  params[2] = LLVMVoidPtrPtrType();
  params[3] = LLVMInt32Type();

  // void *kfunc_call(void *self, void *ob, void *args[], int32_t argc);
  proto = LLVMFunctionType(LLVMVoidPtrType(), params, 4, 0);
  gstate.call = llvm_cfunction("kfunc_call", proto, kfunc_call);

  // void kobj_decref(void *self);
  proto = LLVMFunctionType(LLVMVoidType(), params, 1, 0);
  gstate.decref = llvm_cfunction("kobj_decref", proto, kobj_decref);

  // void kobj_incref(void *self);
  gstate.decref = llvm_cfunction("kobj_incref", proto, kobj_incref);

  // void jit_initargs(void *args[], int32_t argc);
  params[0] = LLVMVoidPtrPtrType();
  params[1] = LLVMInt32Type();
  proto = LLVMFunctionType(LLVMVoidType(), params, 2, 0);
  gstate.initargs = llvm_cfunction("jit_initargs", proto, jit_initargs);

  // void jit_clearstk(void *args[], int32_t argc);
  proto = LLVMFunctionType(LLVMVoidType(), params, 2, 0);
  gstate.clearstk = llvm_cfunction("jit_clearstk", proto, jit_clearstk);
}

void init_jit_llvm(void)
{
  init_types();
  init_wheels();
}

void fini_jit_llvm(void)
{
  if (gstate.engine != NULL)
    LLVMDisposeExecutionEngine(gstate.engine);
  LLVMShutdown();
}

JITValue *jit_const(Object *ob)
{
  JITValue *val = kmalloc(sizeof(JITValue));
  val->konst = 1;
  val->ob = ob;

  if (integer_check(ob)) {
    val->type = JITIntType;
    int64_t ival = integer_asint(ob);
    val->llvalue = LLVMConstInt(JITIntType.lltype, ival, 1);
  } else if (byte_check(ob)) {
    val->type = JITByteType;
    int8_t bval = byte_asint(ob);
    val->llvalue = LLVMConstInt(JITByteType.lltype, bval, 0);
  } else if (bool_check(ob)) {
    val->type = JITBoolType;
    int8_t zval = bool_istrue(ob);
    val->llvalue = LLVMConstInt(JITBoolType.lltype, zval, 0);
  } else if (float_check(ob)) {
    val->type = JITFloatType;
    double fval = float_asflt(ob);
    val->llvalue = LLVMConstReal(JITFloatType.lltype, fval);
  } else if (char_check(ob)) {
    val->type = JITCharType;
    int32_t ival = char_asch(ob);
    val->llvalue = LLVMConstInt(JITCharType.lltype, ival, 0);
  } else if (string_check(ob)) {
    // LLVMConstString is (u8*) ?
    val->type = JITStrType;
    char *sval = string_asstr(ob);
    val->llvalue = LLVMConstString(sval, strlen(sval), 0);
  } else {
    panic("MUST not here!!");
  }

  return val;
}

JITValue *jit_variable(char *name, JITType type)
{
  JITValue *var = kmalloc(sizeof(JITValue));
  var->konst = 0;
  var->type = type;
  var->llvalue = NULL;
  var->name = name;
  return var;
}

JITValue *jit_param(char *name, JITType type, LLVMValueRef val)
{
  JITValue *var = jit_variable(name, type);
  var->llvalue = val;
  return var;
}

void jit_free_value(JITValue *val)
{
  kfree(val);
}

static void jit_decref(LLVMBuilderRef builder, JITValue *val)
{
  LLVMFunction *cf = &gstate.decref;
  LLVMBuildCall2(builder, cf->proto, cf->llfunc, &val->llvalue, 1, cf->name);
}

static
void jit_store_value(LLVMBuilderRef builder, JITValue *dest, JITValue *src)
{
  if (dest->konst) {
    panic("[JIT]: target of store is constant");
  }

  JITType dtype = dest->type;
  JITType stype = src->type;
  if (dtype.str != stype.str) {
    panic("[JIT]: types of target and source are not the same");
  }

  if (dest->llvalue == NULL) {
    dest->llvalue = LLVMBuildAlloca(builder, dtype.lltype, dest->name);
  } else {
    if (!dtype.basic) {
      // decrease object refcnt
      jit_decref(builder, dest);
    }
  }
  LLVMBuildStore(builder, src->llvalue, dest->llvalue);
  dest->ob = src->ob;
}

LLVMFunction llvm_function(char *name, LLVMTypeRef proto)
{
  LLVMValueRef llfunc = LLVMAddFunction(llvm_module(), name, proto);
  LLVMFunction func = {name, proto, llfunc};
  return func;
}

LLVMFunction llvm_cfunction(char *name, LLVMTypeRef proto, void *addr)
{
  LLVMFunction func = llvm_function(name, proto);
  LLVMSetLinkage(func.llfunc, LLVMExternalLinkage);
  LLVMAddGlobalMapping(llvm_engine(), func.llfunc, addr);
  return func;
}

JITFunction *jit_function(CodeObject *co)
{
  JITFunction *f = kmalloc(sizeof(JITFunction));
  Vector *args = co->proto->proto.args;
  int nargs = vector_size(args);

  gvector_init(&f->argtypes, nargs, sizeof(JITType));
  LLVMTypeRef params[nargs];

  JITType type;
  TypeDesc *desc;
  vector_for_each(desc, args) {
    type = jit_type(desc);
    gvector_push_back(&f->argtypes, &type);
    params[idx] = type.lltype;
  }

  desc = co->proto->proto.ret;
  type = jit_type(desc);
  f->rettype = type;
  LLVMTypeRef ret = type.lltype;

  LLVMTypeRef llproto = LLVMFunctionType(ret, params, nargs, 0);;
  f->func = llvm_function(function_name(co), llproto);
  f->co = co;

  // create default block
  JITBlock *blk = jit_block("entry");
  vector_push_back(&f->blocks, blk);
  f->root = blk;
  return f;
}

void jit_free_function(JITFunction *f)
{
  gvector_fini(&f->argtypes);
  kfree(f);
}

void jit_context_init(JITContext *ctx, JITFunction *f)
{
  CodeObject *co = f->co;

  int num = vector_size(&co->locvec);
  vector_init_capacity(&ctx->locals, num);

  Vector *locvars = &co->locvec;
  LocVar *locvar;
  JITType type;
  JITValue *var;
  for (int i = 0; i < num; ++i) {
    locvar = vector_get(locvars, i);
    type = jit_type(locvar->desc);
    var = jit_variable(locvar->name, type);
    vector_set(&ctx->locals, i + 1, var);
  }

  LLVMValueRef llfunc = f->func.llfunc;
  int args = LLVMCountParams(llfunc);
  LLVMValueRef llvalue;
  for (int i = 0; i < args; ++i) {
    var = vector_get(&ctx->locals, i + 1);
    llvalue = LLVMGetParam(llfunc, i);
    var->llvalue = llvalue;
    LLVMSetValueName2(llvalue, var->name, strlen(var->name));
  }

  vector_init(&ctx->stack);
  ctx->func = f;
  ctx->curblk = 0;
}

void jit_context_fini(JITContext *ctx)
{
  vector_fini(&ctx->locals);
  expect(vector_size(&ctx->stack) == 0);
  vector_fini(&ctx->stack);
}

JITBlock *jit_block(char *label)
{
  JITBlock *blk = kmalloc(sizeof(JITBlock));
  blk->label = label;
  return blk;
}

void jit_free_block(JITBlock *blk)
{
  free(blk);
}

void jit_block_add(JITBlock *blk, JITInstruction *inst)
{
  vector_push_back(&blk->insts, inst);
}

#define JIT_ADD  1
#define JIT_SUB  2
#define JIT_MUL  3
#define JIT_RET  4

const char *opnames[] = {
  NULL,
  "add",
  "sub",
  "mul",
  "ret",
};

static inline
JITInstruction *jit_inst(int op, JITValue *lhs, JITValue *rhs, JITValue *ret)
{
  JITInstruction *inst = kmalloc(sizeof(JITInstruction));
  inst->op = op;
  inst->opname = opnames[op];
  inst->lhs = lhs;
  inst->rhs = rhs;
  inst->ret = ret;
  return inst;
}

void jit_free_inst(JITInstruction *inst)
{
  kfree(inst);
}

JITInstruction *jit_inst_add(JITBlock *blk, JITValue *lhs, JITValue *rhs)
{
  JITInstruction *inst = jit_inst(JIT_ADD, lhs, rhs, NULL);
  inst->ret = jit_variable("tmp", lhs->type);
  jit_block_add(blk, inst);
  return inst;
}

JITInstruction *jit_inst_sub(JITBlock *blk, JITValue *lhs, JITValue *rhs)
{
  JITInstruction *inst = jit_inst(JIT_SUB, lhs, rhs, NULL);
  inst->ret = jit_variable("tmp", lhs->type);
  jit_block_add(blk, inst);
  return inst;
}

JITInstruction *jit_inst_ret(JITBlock *blk, JITValue *ret)
{
  JITInstruction *inst = jit_inst(JIT_RET, NULL, NULL, ret);
  jit_block_add(blk, inst);
  return inst;
}

/*
void maybe_handle_refcnt(LLVMBuilderRef builder, Vector *locals)
{
  VECTOR(todos);
  JITValue *val;
  vector_for_each(val, locals) {
    if (val->ob != NULL) {
      vector_push_back(&todos, val->ob);
    }
  }
  LLVMBuildArrayMalloc(builder, jit_type_voidptr(), );
  LLVMBuildCall();
  vector_fini(&todos);
}
*/

static void jit_verify_ir(void)
{
  char *errmsg = NULL;
  int res = LLVMVerifyModule(llvm_module(), LLVMReturnStatusAction, &errmsg);
  if (res && errmsg != NULL) {
    error("%s", errmsg);
  }

//#if !defined(NLog)
  LLVMDumpModule(llvm_module());
//#endif
}

void jit_emit_ir(JITContext *ctx)
{
  JITFunction *fn = ctx->func;
  LLVMFunction *func = &fn->func;
  JITInstruction *inst;
  JITBlock *blk;
  LLVMValueRef lhs;
  LLVMValueRef rhs;

  LLVMBuilderRef builder = LLVMCreateBuilder();
  vector_for_each(blk, &fn->blocks) {
    if (blk->bb == NULL) {
      blk->bb = LLVMAppendBasicBlock(func->llfunc, blk->label);
    }
    LLVMPositionBuilderAtEnd(builder, blk->bb);
    vector_for_each(inst, &blk->insts) {
      switch (inst->op) {
      case JIT_ADD: {
        lhs = inst->lhs->llvalue;
        rhs = inst->rhs->llvalue;
        lhs = LLVMBuildAdd(builder, lhs, rhs, "tmp");
        inst->ret->llvalue = lhs;
        break;
      }
      case JIT_RET: {
        // reference counting
        //maybe_handle_refcnt(builder, &ctx->locals);
        LLVMBuildRet(builder, inst->ret->llvalue);
        break;
      }
      default:
        panic("MUST not here!!");
        break;
      }
    }
  }
  LLVMDisposeBuilder(builder);

  // verify ir
  jit_verify_ir();
}
