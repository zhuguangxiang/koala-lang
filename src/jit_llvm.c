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

JitType JitVoidType   = {NULL, 0, "void"};
JitType JitIntType    = {NULL, 1, "int"  };
JitType JitByteType   = {NULL, 1, "byte" };
JitType JitFloatType  = {NULL, 1, "float"};
JitType JitBoolType   = {NULL, 1, "bool" };
JitType JitCharType   = {NULL, 1, "char" };
JitType JitStrType    = {NULL, 0, "str"  };
JitType JitKlassType  = {NULL, 0, "klass"};

static void init_types(void)
{
  JitVoidType.lltype  = LLVMVoidType();
  JitIntType.lltype   = LLVMInt64Type();
  JitByteType.lltype  = LLVMInt8Type();
  JitFloatType.lltype = LLVMDoubleType();
  JitBoolType.lltype  = LLVMInt8Type();
  JitCharType.lltype  = LLVMInt32Type();
  JitStrType.lltype   = LLVMVoidPtrType();
  JitKlassType.lltype = LLVMVoidPtrType();
}

JitType jit_type(TypeDesc *desc)
{
  if (desc == NULL) {
    return JitVoidType;
  }

  if (!desc_isbase(desc)) {
    return JitKlassType;
  }

  JitType type = {NULL, 1, NULL};
  int kind = desc->base;
  switch (kind) {
  case BASE_INT:
    type = JitIntType;
    break;
  case BASE_BYTE:
    type = JitByteType;
    break;
  case BASE_FLOAT:
    type = JitFloatType;
    break;
  case BASE_BOOL:
    type = JitBoolType;
    break;
  case BASE_STR:
    type = JitStrType;
    break;
  case BASE_CHAR:
    type = JitCharType;
    break;
  case BASE_ANY:
    type = JitKlassType;
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

static void jit_enterfunc(void *args[], int32_t argc)
{

}

static void jit_exitfunc(void *args[], int32_t argc)
{

}

static Object *__new_tuple__(Object *args[], int argc)
{

}

static JitState gstate;

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

static void init_wheels(void)
{
  LLVMTypeRef proto;
  LLVMTypeRef params[4];
  params[0] = LLVMVoidPtrType();
  params[1] = LLVMVoidPtrType();
  params[2] = LLVMVoidPtrPtrType();
  params[3] = LLVMInt32Type();

  // Object *array_new(TypeDesc *desc, GVector *vec);
  proto = LLVMFunctionType(LLVMVoidPtrType(), params, 2, 0);
  gstate.newarray = llvm_cfunction("newarray", proto, array_new);

  // Object *map_new(TypeDesc *ktype, TypeDesc *vtype);
  proto = LLVMFunctionType(LLVMVoidPtrType(), params, 2, 0);
  gstate.newmap = llvm_cfunction("newmap", proto, map_new);

  // Object *object_alloc(typeObject *type);
  proto = LLVMFunctionType(LLVMVoidPtrType(), params, 1, 0);
  gstate.newobject = llvm_cfunction("newobject", proto, object_alloc);

  // void *kfunc_call(Object *self, Object *ob, void *args[], int32_t argc);
  proto = LLVMFunctionType(LLVMVoidPtrType(), params, 4, 0);
  gstate.call = llvm_cfunction("kfunc_call", proto, kfunc_call);

  // void kobj_decref(Object *self);
  proto = LLVMFunctionType(LLVMVoidType(), params, 1, 0);
  gstate.decref = llvm_cfunction("obj_decref", proto, kobj_decref);

  // void kobj_incref(Object *self);
  proto = LLVMFunctionType(LLVMVoidType(), params, 1, 0);
  gstate.incref = llvm_cfunction("obj_incref", proto, kobj_incref);

  // void jit_enterfunc(Object *args[], int32_t argc);
  params[0] = LLVMVoidPtrPtrType();
  params[1] = LLVMInt32Type();
  proto = LLVMFunctionType(LLVMVoidType(), params, 2, 0);
  gstate.enterfunc = llvm_cfunction("enterfunc", proto, jit_enterfunc);

  // void jit_exitfunc(Object *args[], int32_t argc);
  proto = LLVMFunctionType(LLVMVoidType(), params, 2, 0);
  gstate.exitfunc = llvm_cfunction("exitfunc", proto, jit_exitfunc);

  // Object *__new_tuple__(Object *args[], int argc);
  proto = LLVMFunctionType(LLVMVoidPtrType(), params, 2, 0);
  gstate.newtuple = llvm_cfunction("newtuple", proto, __new_tuple__);
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

LLVMValueRef jit_const(Object *ob)
{
  if (integer_check(ob)) {
    int64_t ival = integer_asint(ob);
    return LLVMConstInt(JitIntType.lltype, ival, 1);
  } else if (byte_check(ob)) {
    int8_t bval = byte_asint(ob);
    return LLVMConstInt(JitByteType.lltype, bval, 0);
  } else if (bool_check(ob)) {
    int8_t zval = bool_istrue(ob);
    return LLVMConstInt(JitBoolType.lltype, zval, 0);
  } else if (float_check(ob)) {
    double fval = float_asflt(ob);
    return LLVMConstReal(JitFloatType.lltype, fval);
  } else if (char_check(ob)) {
    int32_t ival = char_asch(ob);
    return LLVMConstInt(JitCharType.lltype, ival, 0);
  } else if (string_check(ob)) {
    // LLVMConstString is (u8*) ?
    char *sval = string_asstr(ob);
    return LLVMConstString(sval, strlen(sval), 0);
  } else {
    panic("MUST not here!!");
    return NULL;
  }
}

JitValue *jit_value(char *name, JitType type)
{
  JitValue *var = kmalloc(sizeof(JitValue));
  var->konst = 0;
  var->type = type;
  var->llvalue = NULL;
  var->name = name;
  return var;
}

void jit_free_value(JitValue *val)
{
  kfree(val);
}

static void jit_decref(JitContext *ctx, JitValue *var)
{
  printf("decref object:%s\n", var->name);
  LLVMBuilderRef builder = ctx->builder;
  LLVMFunction *cf = &gstate.decref;
  LLVMValueRef params[] = {var->llvalue};
  LLVMBuildCall2(builder, cf->proto, cf->llfunc, params, 1, "");
}

void jit_store_value(JitContext *ctx, JitValue *var, LLVMValueRef val)
{
  LLVMBuilderRef builder = ctx->builder;
  JitType type = var->type;
  if (!type.basic && var->llvalue != NULL) {
    // decrease object refcnt
    jit_decref(ctx, var);
  }
  var->llvalue = val;
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
    strbuf_append_char(&sbuf, '.');
    if (co->type != NULL) {
      strbuf_append(&sbuf, co->type->name);
      strbuf_append_char(&sbuf, '.');
    }
    strbuf_append(&sbuf, co->name);
    name = atom(strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
    co->longname = name;
  }
  return name;
}

void *jit_emit_code(JitFunction *func)
{
  char *name = function_name(func->co);
  return (void *)LLVMGetFunctionAddress(llvm_engine(), name);
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

JitFunction *jit_function(CodeObject *co)
{
  JitFunction *f = kmalloc(sizeof(JitFunction));
  Vector *args = co->proto->proto.args;
  int nargs = vector_size(args);

  gvector_init(&f->argtypes, nargs, sizeof(JitType));
  LLVMTypeRef params[nargs];

  JitType type;
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

  return f;
}

void jit_free_function(JitFunction *f)
{
  gvector_fini(&f->argtypes);
  kfree(f);
}

void jit_context_init(JitContext *ctx, JitFunction *f)
{
  CodeObject *co = f->co;

  vector_init(&ctx->stack);
  ctx->func = f;
  // create default block
  ctx->curblk = 0;
  vector_init(&ctx->blocks);
  JitBlock *blk = jit_block(ctx, "entry");
  ctx->builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(ctx->builder, blk->bb);

  int num = vector_size(&co->locvec);
  vector_init_capacity(&ctx->locals, num);
  // initialize local variables, include arguments.
  Vector *locvars = &co->locvec;
  LocVar *locvar;
  JitType type;
  JitValue *var;
  for (int i = 0; i < num; ++i) {
    locvar = vector_get(locvars, i);
    type = jit_type(locvar->desc);
    var = jit_value(locvar->name, type);
    vector_set(&ctx->locals, i + 1, var);
  }

  // initialize arguments
  LLVMValueRef llfunc = f->func.llfunc;
  int args = LLVMCountParams(llfunc);
  LLVMValueRef llvalue;
  for (int i = 0; i < args; ++i) {
    var = vector_get(&ctx->locals, i + 1);
    llvalue = LLVMGetParam(llfunc, i);
    LLVMSetValueName2(llvalue, var->name, strlen(var->name));
    var->llvalue = llvalue;
  }
}

void jit_context_fini(JitContext *ctx)
{
  LLVMDisposeBuilder(ctx->builder);
  vector_fini(&ctx->locals);
  expect(vector_size(&ctx->stack) == 0);
  vector_fini(&ctx->stack);
}

JitBlock *jit_block(JitContext *ctx, char *label)
{
  JitFunction *f = ctx->func;
  LLVMFunction *func = &f->func;
  JitBlock *blk = kmalloc(sizeof(JitBlock));
  blk->label = label;
  blk->index = vector_size(&ctx->blocks);
  blk->func = f;
  blk->bb = LLVMAppendBasicBlock(func->llfunc, blk->label);
  vector_push_back(&ctx->blocks, blk);
  return blk;
}

void jit_free_block(JitBlock *blk)
{
  free(blk);
}

/*
void maybe_handle_refcnt(LLVMBuilderRef builder, Vector *locals)
{
  VECTOR(todos);
  JitValue *val;
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

void jit_verify_ir(void)
{
  char *errmsg = NULL;
  int res = LLVMVerifyModule(llvm_module(), LLVMReturnStatusAction, &errmsg);
  if (res && errmsg != NULL) {
    error("%s", errmsg);
  }

  if (errmsg != NULL) {
    free(errmsg);
  }

//#if !defined(NLog)
  LLVMDumpModule(llvm_module());
//#endif
}

static LLVMValueRef LLVMConstVoidPtr(void *ptr)
{
  LLVMValueRef llvalue = LLVMConstInt(LLVMIntPtrType2(), (intptr_t)ptr, 0);
  return LLVMConstIntToPtr(llvalue, LLVMVoidPtrType());
}

static
LLVMValueRef jit_new_array(JitContext *ctx, TypeDesc *sub)
{
  LLVMBuilderRef builder = ctx->builder;
  LLVMFunction *cf = &gstate.newarray;
  LLVMValueRef params[] = {
    LLVMConstVoidPtr(sub),
    LLVMConstNull(LLVMVoidType()),
  };
  LLVMValueRef llvalue;
  llvalue = LLVMBuildCall2(builder, cf->proto, cf->llfunc, params, 2, "");
  return llvalue;
}

static
LLVMValueRef jit_new_map(JitContext *ctx, TypeDesc *ktype, TypeDesc *vtype)
{
  LLVMBuilderRef builder = ctx->builder;
  LLVMFunction *cf = &gstate.newmap;
  LLVMValueRef params[] = {
    LLVMConstVoidPtr(ktype),
    LLVMConstVoidPtr(vtype),
  };
  LLVMValueRef llvalue;
  llvalue = LLVMBuildCall2(builder, cf->proto, cf->llfunc, params, 2, "");
  return llvalue;
}

static LLVMValueRef jit_new_object(JitContext *ctx, TypeObject *type)
{
  LLVMBuilderRef builder = ctx->builder;
  LLVMFunction *cf = &gstate.newobject;
  LLVMValueRef params[] = {
    LLVMConstVoidPtr(type),
  };
  LLVMValueRef llvalue;
  llvalue = LLVMBuildCall2(builder, cf->proto, cf->llfunc, params, 1, "");
  return llvalue;
}

LLVMValueRef jit_OP_NEW_TUPLE(JitContext *ctx, Vector *args)
{
  int size = vector_size(args);

  LLVMBuilderRef builder = ctx->builder;
  LLVMFunction *cf = &gstate.newtuple;
  LLVMValueRef params[] = {
    NULL,
    LLVMConstInt(LLVMInt32Type(), size, 1),
  };
  LLVMValueRef llvalue;
  llvalue = LLVMBuildCall2(builder, cf->proto, cf->llfunc, params, 2, "");
  return llvalue;
}

LLVMValueRef jit_OP_NEW(JitContext *ctx, CodeObject *co, TypeDesc *desc)
{
  Object *mob = module_load(desc->klass.path);
  if (mob == NULL) {
    mob = OB_INCREF(co->module);
  }
  expect(mob != NULL);

  Object *kob;
  kob = module_lookup(mob, desc->klass.type);
  expect(kob != NULL);
  OB_DECREF(mob);

  LLVMValueRef ret;
  TypeObject *type = (TypeObject *)kob;
  if (type == &array_type) {
    TypeDesc *sub = vector_get(desc->klass.typeargs, 0);
    ret = jit_new_array(ctx, sub);
  } else if (type == &map_type) {
    TypeDesc *kdesc = vector_get(desc->klass.typeargs, 0);
    TypeDesc *vdesc = vector_get(desc->klass.typeargs, 1);
    ret = jit_new_map(ctx, kdesc, vdesc);
  } else {
    expect(type->alloc != NULL);
    ret = jit_new_object(ctx, type);
  }
  OB_DECREF(kob);
  return ret;
}
