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

#define WHEEL_NEW_TUPLE   0
#define WHEEL_NEW_ARRAY   1
#define WHEEL_NEW_MAP     2
#define WHEEL_NEW_OBJECT  3
#define MAX_WHEEL_NR      4

static JitFunction wheel_funcs[MAX_WHEEL_NR];
static LLVMExecutionEngineRef llengine;
static LLVMModuleRef llmod;

#define JIT_MODULE_NAME "__koala_jitter__"

static LLVMModuleRef llvm_module(void)
{
  if (llmod == NULL) {
    llmod = LLVMModuleCreateWithName(JIT_MODULE_NAME);
  }
  return llmod;
}

static LLVMExecutionEngineRef llvm_engine(void)
{
  if (llengine == NULL) {
    char *error;
    LLVMExecutionEngineRef engine;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMCreateExecutionEngineForModule(&engine, llvm_module(), &error);
    llengine = engine;
  }
  return llengine;
}

/*--------------------------------------------------------------------------*/

static JitType JitIntType   = {NULL, 'i'};
static JitType JitByteType  = {NULL, 'b'};
static JitType JitFloatType = {NULL, 'f'};
static JitType JitBoolType  = {NULL, 'z'};
static JitType JitCharType  = {NULL, 'c'};
static JitType JitStrType   = {NULL, 's'};
static JitType JitVoidType  = {NULL, 'v'};
static JitType JitPtrType   = {NULL, 'p'};
static JitType JitArgsType  = {NULL, 'a'};
static JitType JitNrType    = {NULL, 'n'};

static void init_types(void)
{
  JitVoidType.lltype  = LLVMVoidType();
  JitIntType.lltype   = LLVMInt64Type();
  JitByteType.lltype  = LLVMInt8Type();
  JitFloatType.lltype = LLVMDoubleType();
  JitBoolType.lltype  = LLVMInt8Type();
  JitCharType.lltype  = LLVMInt32Type();
  JitStrType.lltype   = LLVMVoidPtrType();
  JitPtrType.lltype   = LLVMVoidPtrType();
  JitArgsType.lltype  = LLVMVoidPtrPtrType();
  JitNrType.lltype    = LLVMInt32Type();
}

JitType jit_type(TypeDesc *desc)
{
  if (desc == NULL) {
    return JitVoidType;
  }

  if (!desc_isbase(desc)) {
    return JitPtrType;
  }

  JitType type = JitVoidType;
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
    type = JitPtrType;
    break;
  default:
    panic("why go here?");
    break;
  }
  return type;
}

/*--------------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------------*/

/* NOTE: Parameter 'name' is c function name with external attribute */
static void jit_init_cfunc(JitFunction *func, char *name,
                          JitType rtype, JitType *ptypes, int size)
{
  gvector_init(&func->argtypes, 8, sizeof(JitType));
  LLVMTypeRef params[size];
  for (int i = 0; i < size; ++i) {
    params[i] = ptypes[i].lltype;
    gvector_push_back(&func->argtypes, &ptypes[i]);
  }

  LLVMTypeRef llproto = LLVMFunctionType(rtype.lltype, params, size, 0);
  LLVMValueRef llfunc  = LLVMAddFunction(llvm_module(), name, llproto);
  LLVMSetLinkage(llfunc, LLVMExternalLinkage);
  //LLVMAddGlobalMapping(llvm_engine(), llfunc, addr);

  func->name    = name;
  func->rettype = rtype;
  func->llproto = llproto;
  func->llfunc  = llfunc;
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

static Object *__obj_box__(int kind, void *ptr)
{
  Object *ob = NULL;
  switch (kind) {
  case 'i':
    ob = integer_new((int64_t)(intptr_t)ptr);
    break;
  case 'b':
    ob = byte_new((int)(intptr_t)ptr);
    break;
  case 'c':
    ob = char_new((unsigned int)(intptr_t)ptr);
    break;
  case 'z':
    ob = ptr ? bool_true() : bool_false();
    break;
  case 'f':
    ob = float_new((double)(intptr_t)ptr);
    break;
  case 's':
  case 'p':
    ob = OB_INCREF(ptr);
    break;
  default:
    panic("No go here!!");
    break;
  }
  return ob;
}

Object *__new_tuple__(void *args[], int32_t kind[], int32_t argc)
{
  Object *tuple = tuple_new(argc);
  int ikind;
  void *value;
  Object *ob;
  for (int i = 0; i < argc; ++i) {
    ikind = kind[i];
    value = args[i];
    ob = __obj_box__(ikind, value);
    tuple_set(tuple, i, ob);
    OB_DECREF(ob);
  }
  return tuple;
}

Object *__new_array__(TypeDesc *type, void *args[], int32_t argc)
{
  GVector *vec = gvector_new(argc, sizeof(RawValue));
  RawValue raw;
  int isobj = 0;

  if (type->kind != TYPE_BASE) {
    isobj = 1;
  } else if (type->base == BASE_STR || type->base == BASE_ANY) {
    isobj = 1;
  } else {
    isobj = 0;
  }

  if (isobj) {
    for (int i = 0; i < argc; i++) {
      raw.obj = args[i];
      OB_INCREF(raw.obj);
      gvector_set(vec, i, &raw);
    }
  } else {
    for (int i = 0; i < argc; i++) {
      raw.obj = args[i];
      gvector_set(vec, i, &raw);
    }
  }

  return array_new(type, vec);
}

Object *__new_map__(TypeDesc *ktype, TypeDesc *vtype,
                    void *args[], int32_t argc)
{
  Object *map = map_new(ktype, vtype);
  JitType key = jit_type(ktype);
  JitType val = jit_type(vtype);
  Object *kob, *vob;
  void *kptr, *vptr;
  for (int i = 0; i < argc; i += 2) {
    kptr = args[i];
    vptr = args[i+1];
    kob = __obj_box__(key.kind, kptr);
    vob = __obj_box__(val.kind, vptr);
    map_put(map, kob, vob);
    OB_DECREF(kob);
    OB_DECREF(vob);
  }
  return map;
}

/* Object *__new_tuple__(void *args[], int32_t kind[], int32_t argc) */
static void init_new_tuple(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_NEW_TUPLE];
  JitType params[] = {
    JitArgsType,
    JitArgsType,
    JitNrType,
  };
  JitType ret = JitPtrType;
  jit_init_cfunc(f, "__new_tuple__", ret, params, 3);
}

/* Object *__new_array__(TypeDesc *type, void *args[], int32_t argc) */
static void init_new_array(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_NEW_ARRAY];
  JitType params[] = {
    JitPtrType,
    JitArgsType,
    JitNrType,
  };
  JitType ret = JitPtrType;
  jit_init_cfunc(f, "__new_array__", ret, params, 3);
}

/*
  Object *__new_map__(TypeDesc *ktype, TypeDesc *vtype, void *args[], int argc);
 */
static void init_new_map(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_NEW_MAP];
  JitType params[] = {
    JitPtrType,
    JitPtrType,
    JitArgsType,
    JitNrType,
  };
  JitType ret = JitPtrType;
  jit_init_cfunc(f, "__new_map__", ret, params, 4);
}

/* Object *object_alloc(typeObject *type); */
static void init_new_object(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_NEW_OBJECT];
  JitType params[] = {
    JitPtrType,
  };
  JitType ret = JitPtrType;
  jit_init_cfunc(f, "object_alloc", ret, params, 1);
}

static void init_wheel_funcs(void)
{
  init_new_tuple();
  init_new_array();
  init_new_map();
  init_new_object();

  /*
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
  */
}

static void fini_wheel_funcs(void)
{
  JitFunction *f;
  for (int i = 0; i < MAX_WHEEL_NR; ++i) {
    f = &wheel_funcs[i];
    gvector_fini(&f->argtypes);
  }
}

static void jit_decref(JitContext *ctx, JitVariable *var)
{
  printf("decref object:%s\n", var->name);
  LLVMBuilderRef builder = ctx->builder;
  //LLVMFunction *cf = &gstate.decref;
  //LLVMValueRef params[] = {var->llvalue};
  //LLVMBuildCall(builder, cf->llfunc, params, 1, "");
}

/*--------------------------------------------------------------------------*/

void init_jit_llvm(void)
{
  init_types();
  init_wheel_funcs();
}

void fini_jit_llvm(void)
{
  fini_wheel_funcs();

  if (llengine != NULL) {
    LLVMDisposeExecutionEngine(llengine);
  }
  LLVMShutdown();
}

/*--------------------------------------------------------------------------*/

static JitValue jit_const_int(int64_t ival)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitIntType.lltype, ival, 1);
  value.type = JitIntType;
  return value;
}

static JitValue jit_const_byte(int8_t bval)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitByteType.lltype, bval, 0);
  value.type = JitByteType;
  return value;
}

static JitValue jit_const_bool(int8_t zval)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitBoolType.lltype, zval, 0);
  value.type = JitBoolType;
  return value;
}

static JitValue jit_const_float(double fval)
{
  JitValue value;
  value.llvalue = LLVMConstReal(JitFloatType.lltype, fval);
  value.type = JitFloatType;
  return value;
}

static JitValue jit_const_char(int32_t ival)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitCharType.lltype, ival, 1);
  value.type = JitCharType;
  return value;
}

static JitValue jit_const_str(void *ptr)
{
  JitValue value;
  LLVMValueRef llvalue = LLVMConstInt(LLVMIntPtrType2(), (intptr_t)ptr, 0);
  value.llvalue = LLVMConstIntToPtr(llvalue, JitStrType.lltype);
  value.type = JitStrType;
  return value;
}

static JitValue jit_const_ptr(void *ptr)
{
  JitValue value;
  LLVMValueRef llvalue = LLVMConstInt(LLVMIntPtrType2(), (intptr_t)ptr, 0);
  value.llvalue = LLVMConstIntToPtr(llvalue, JitPtrType.lltype);
  value.type = JitPtrType;
  return value;
}

static JitValue jit_const_num(int32_t ival)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitNrType.lltype, ival, 1);
  value.type = JitNrType;
  return value;
}

static JitValue jit_const(Object *ob)
{
  JitValue value;
  if (integer_check(ob)) {
    int64_t ival = integer_asint(ob);
    value = jit_const_int(ival);
  } else if (byte_check(ob)) {
    int8_t bval = byte_asint(ob);
    value = jit_const_byte(bval);
  } else if (bool_check(ob)) {
    int8_t zval = bool_istrue(ob);
    value = jit_const_bool(zval);
  } else if (float_check(ob)) {
    double fval = float_asflt(ob);
    value = jit_const_float(fval);
  } else if (char_check(ob)) {
    int32_t ival = char_asch(ob);
    value = jit_const_char(ival);
  } else if (string_check(ob)) {
    value = jit_const_str(ob);
  } else {
    panic("MUST not here!!");
  }
  return value;
}

JitVariable *jit_variable(char *name, JitType type)
{
  JitVariable *var = kmalloc(sizeof(JitVariable));
  var->type = type;
  var->llvalue = NULL;
  var->name = name;
  return var;
}

void jit_free_variable(JitVariable *var)
{
  kfree(var);
}

static inline int jit_has_object(JitVariable *var)
{
  JitType *type = &var->type;
  int isobj = type->kind == 'p' || type->kind == 's';
  return isobj && var->llvalue != NULL;
}

static LLVMValueRef jit_value_to_voidptr(JitContext *ctx, JitValue *val)
{
  LLVMBuilderRef builder = ctx->builder;
  int kind = val->type.kind;
  LLVMValueRef llvalue;
  switch (kind) {
  case 'i':
  case 'b':
  case 'c':
  case 'z':
    llvalue = LLVMBuildIntToPtr(builder, val->llvalue, LLVMVoidPtrType(), "");
    break;
  case 's':
  case 'p':
    llvalue = val->llvalue;
    break;
  case 'f':
    llvalue = LLVMBuildFPCast(builder, val->llvalue, LLVMVoidPtrType(), "");
    break;
  default:
    panic("NOT go here!!");
    break;
  }
  return llvalue;
}

static LLVMValueRef jit_type_to_int(JitContext *ctx, JitValue *val)
{
  int kind = val->type.kind;
  return jit_const_num(kind).llvalue;
}

/*--------------------------------------------------------------------------*/

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

  f->llproto = LLVMFunctionType(ret, params, nargs, 0);
  f->llfunc  = LLVMAddFunction(llvm_module(), function_name(co), f->llproto);

  return f;
}

void jit_free_function(JitFunction *f)
{
  gvector_fini(&f->argtypes);
  kfree(f);
}

/*--------------------------------------------------------------------------*/

void jit_context_init(JitContext *ctx, CodeObject *co)
{
  gvector_init(&ctx->stack, 64, sizeof(JitValue));
  JitFunction *f = jit_function(co);
  ctx->co = co;
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
  JitVariable *var;
  for (int i = 0; i < num; ++i) {
    locvar = vector_get(locvars, i);
    type = jit_type(locvar->desc);
    var = jit_variable(locvar->name, type);
    vector_set(&ctx->locals, i + 1, var);
  }

  // initialize arguments
  LLVMValueRef llfunc = f->llfunc;
  int args = LLVMCountParams(llfunc);
  LLVMValueRef llvalue;
  for (int i = 0; i < args; ++i) {
    var = vector_get(&ctx->locals, i + 1);
    llvalue = LLVMGetParam(llfunc, i);
    LLVMSetValueName(llvalue, var->name);
    var->llvalue = llvalue;
  }
}

void jit_context_fini(JitContext *ctx)
{
  LLVMDisposeBuilder(ctx->builder);

  JitBlock *blk;
  vector_for_each(blk, &ctx->blocks) {
    jit_free_block(blk);
  }
  vector_fini(&ctx->blocks);

  JitVariable *var;
  vector_for_each(var, &ctx->locals) {
    jit_free_variable(var);
  }
  vector_fini(&ctx->locals);

  expect(vector_size(&ctx->stack) == 0);
  gvector_fini(&ctx->stack);

  jit_free_function(ctx->func);
}

void *jit_emit_code(JitContext *ctx)
{
  char *name = function_name(ctx->co);
  return (void *)LLVMGetFunctionAddress(llvm_engine(), name);
}

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

/*--------------------------------------------------------------------------*/

JitBlock *jit_block(JitContext *ctx, char *label)
{
  JitFunction *f = ctx->func;
  JitBlock *blk = kmalloc(sizeof(JitBlock));
  blk->label = label;
  blk->index = vector_size(&ctx->blocks);
  blk->ctx = ctx;
  blk->bb = LLVMAppendBasicBlock(f->llfunc, blk->label);
  vector_push_back(&ctx->blocks, blk);
  return blk;
}

void jit_free_block(JitBlock *blk)
{
  kfree(blk);
}

/*--------------------------------------------------------------------------*/

static inline JitValue
jit_build_call(JitContext *ctx, JitFunction *f, LLVMValueRef *args, int argc)
{
  LLVMBuilderRef builder = ctx->builder;
  LLVMValueRef llvalue = LLVMBuildCall(builder, f->llfunc, args, argc, "");
  JitValue value = {llvalue, f->rettype};
  return value;
}

static LLVMValueRef jit_voidptr_array(JitContext *ctx, GVector *values)
{
  LLVMBuilderRef builder = ctx->builder;
  int size = gvector_size(values);
  int total = size * sizeof(void *);
  LLVMValueRef llsize = LLVMConstInt(LLVMInt64Type(), total, 0);
  LLVMValueRef llargs;
  llargs = LLVMBuildArrayMalloc(builder, LLVMVoidPtrType(), llsize, "");

  JitValue val;
  LLVMValueRef llval;
  LLVMValueRef indices[1];
  LLVMValueRef parg;
  gvector_foreach(val, values) {
    indices[0] = LLVMConstInt(LLVMInt64Type(), idx, 0);
    parg = LLVMBuildInBoundsGEP(builder, llargs, indices, 1, "");
    llval = jit_value_to_voidptr(ctx, &val);
    LLVMBuildStore(builder, llval, parg);
  }
  return llargs;
}

static inline
void jit_free_array(JitContext *ctx, LLVMValueRef llvalue)
{
  LLVMBuilderRef builder = ctx->builder;
  LLVMBuildFree(builder, llvalue);
}

static LLVMValueRef jit_types_array(JitContext *ctx, GVector *values)
{
  LLVMBuilderRef builder = ctx->builder;
  int size = gvector_size(values);
  int total = size * sizeof(void *);
  LLVMValueRef llsize = LLVMConstInt(LLVMInt32Type(), total, 0);
  LLVMValueRef llargs;
  llargs = LLVMBuildArrayMalloc(builder, LLVMInt32Type(), llsize, "");

  JitValue val;
  LLVMValueRef llval;
  LLVMValueRef indices[1];
  LLVMValueRef parg;
  gvector_foreach(val, values) {
    indices[0] = LLVMConstInt(LLVMInt32Type(), idx, 0);
    parg = LLVMBuildInBoundsGEP(builder, llargs, indices, 1, "");
    llval = jit_type_to_int(ctx, &val);
    LLVMBuildStore(builder, llval, parg);
  }
  return llargs;
}

/*--------------------------------------------------------------------------*/

void jit_OP_LOAD_CONST(JitContext *ctx, int index)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);
  JitValue value = jit_const(x);
  gvector_push_back(&ctx->stack, &value);
  OB_DECREF(x);
}

void jit_OP_LOAD(JitContext *ctx, int index)
{
  JitVariable *var = vector_get(&ctx->locals, index);
  JitValue value = *(JitValue *)var;
  gvector_push_back(&ctx->stack, &value);
}

void jit_OP_STORE(JitContext *ctx, int index)
{
  JitValue value;
  gvector_pop_back(&ctx->stack, &value);
  JitVariable *var = vector_get(&ctx->locals, index);

  JitType type = var->type;
  if (jit_has_object(var)) {
    // decrease object refcnt
    jit_decref(ctx, var);
  }
  var->llvalue = value.llvalue;
}

void jit_OP_RETURN_VALUE(JitContext *ctx)
{
  JitValue value;
  gvector_pop_back(&ctx->stack, &value);

  LLVMBuilderRef builder = ctx->builder;
  LLVMBuildRet(builder, value.llvalue);
}

void jit_OP_RETURN(JitContext *ctx)
{
  LLVMBuilderRef builder = ctx->builder;
  LLVMBuildRetVoid(builder);
}

void jit_OP_ADD(JitContext *ctx)
{
  LLVMBuilderRef builder = ctx->builder;
  JitValue lhs;
  gvector_pop_back(&ctx->stack, &lhs);
  JitValue rhs;
  gvector_pop_back(&ctx->stack, &rhs);
  LLVMValueRef llvalue;
  llvalue = LLVMBuildAdd(builder, lhs.llvalue, rhs.llvalue, "");
  JitValue ret;
  ret.llvalue = llvalue;
  ret.type = lhs.type;
  gvector_push_back(&ctx->stack, &ret);
}

/* Object *__new_tuple__(void *args[], int32_t kinds[], int argc); */
static JitValue jit_new_tuple(JitContext *ctx, GVector *args)
{
  LLVMBuilderRef builder = ctx->builder;
  JitFunction *f = &wheel_funcs[WHEEL_NEW_TUPLE];
  int size = gvector_size(args);
  LLVMValueRef llargs = jit_voidptr_array(ctx, args);
  LLVMValueRef lltypes = jit_types_array(ctx, args);
  LLVMValueRef params[] = {
    llargs,
    lltypes,
    jit_const_num(size).llvalue,
  };
  JitValue res = jit_build_call(ctx, f, params, 3);
  jit_free_array(ctx, llargs);
  jit_free_array(ctx, lltypes);
  return res;
}

void jit_OP_NEW_TUPLE(JitContext *ctx, int count)
{
  GVector vec;
  gvector_init(&vec, 16, sizeof(JitValue));
  JitValue value;
  for (int i = 0; i < count; ++i) {
    gvector_pop_back(&ctx->stack, &value);
    gvector_push_back(&vec, &value);
  }

  value = jit_new_tuple(ctx, &vec);
  gvector_push_back(&ctx->stack, &value);
  gvector_fini(&vec);
}

/* Object *__new_array__(TypeDesc *type, void *args[], int32_t argc); */
static JitValue jit_new_array(JitContext *ctx, TypeDesc *sub, GVector *args)
{
  LLVMBuilderRef builder = ctx->builder;
  JitFunction *f = &wheel_funcs[WHEEL_NEW_ARRAY];
  int size = gvector_size(args);
  LLVMValueRef llargs = jit_voidptr_array(ctx, args);
  LLVMValueRef params[] = {
    jit_const_ptr(sub).llvalue,
    llargs,
    jit_const_num(size).llvalue,
  };
  JitValue res = jit_build_call(ctx, f, params, 3);
  jit_free_array(ctx, llargs);
  return res;
}

void jit_OP_NEW_ARRAY(JitContext *ctx, int index, int count)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);
  TypeDesc *desc = descob_getdesc(x);
  OB_DECREF(x);

  GVector vec;
  gvector_init(&vec, 16, sizeof(JitValue));
  JitValue value;
  for (int i = 0; i < count; ++i) {
    gvector_pop_back(&ctx->stack, &value);
    gvector_push_back(&vec, &value);
  }

  TypeDesc *sub = vector_get(desc->klass.typeargs, 0);
  value = jit_new_array(ctx, sub, &vec);
  gvector_push_back(&ctx->stack, &value);
  gvector_fini(&vec);
}

/*
  Object *__new_map__(TypeDesc *ktype, TypeDesc *vtype, void *args[], int argc);
 */
static JitValue jit_new_map(JitContext *ctx, TypeDesc *ktype, TypeDesc *vtype,
                            GVector *args)
{
  LLVMBuilderRef builder = ctx->builder;
  JitFunction *f = &wheel_funcs[WHEEL_NEW_MAP];
  int size = gvector_size(args);
  LLVMValueRef llargs = jit_voidptr_array(ctx, args);
  LLVMValueRef params[] = {
    jit_const_ptr(ktype).llvalue,
    jit_const_ptr(vtype).llvalue,
    llargs,
    jit_const_num(size).llvalue,
  };
  JitValue res = jit_build_call(ctx, f, params, 4);
  jit_free_array(ctx, llargs);
  return res;
}

void jit_OP_NEW_MAP(JitContext *ctx, int index, int count)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);
  TypeDesc *desc = descob_getdesc(x);
  OB_DECREF(x);

  GVector vec;
  gvector_init(&vec, 16, sizeof(JitValue));
  JitValue value;
  for (int i = 0; i < count * 2; ++i) {
    gvector_pop_back(&ctx->stack, &value);
    gvector_push_back(&vec, &value);
  }

  TypeDesc *ksub = vector_get(desc->klass.typeargs, 0);
  TypeDesc *vsub = vector_get(desc->klass.typeargs, 1);
  value = jit_new_map(ctx, ksub, vsub, &vec);
  gvector_push_back(&ctx->stack, &value);
  gvector_fini(&vec);
}

/* Object *object_alloc(typeObject *type); */
static JitValue jit_object_alloc(JitContext *ctx, TypeObject *type)
{
  LLVMBuilderRef builder = ctx->builder;
  JitFunction *f = &wheel_funcs[WHEEL_NEW_OBJECT];
  LLVMValueRef params[] = {
    jit_const_ptr(type).llvalue,
  };
  return jit_build_call(ctx, f, params, 1);
}

void jit_OP_NEW(JitContext *ctx, int index)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);
  TypeDesc *desc = descob_getdesc(x);
  OB_DECREF(x);
  if (desc_isbase(desc)) {
    // basic type, do nothing.
    return;
  }

  Object *mob = module_load(desc->klass.path);
  if (mob == NULL) {
    mob = OB_INCREF(co->module);
  }
  expect(mob != NULL);

  Object *type;
  type = module_lookup(mob, desc->klass.type);
  expect(type != NULL);
  OB_DECREF(mob);

  JitValue ret;
  TypeObject *typo = (TypeObject *)type;
  if (typo == &array_type) {
    TypeDesc *sub = vector_get(desc->klass.typeargs, 0);
    ret = jit_new_array(ctx, sub, NULL);
  } else if (typo == &map_type) {
    TypeDesc *kdesc = vector_get(desc->klass.typeargs, 0);
    TypeDesc *vdesc = vector_get(desc->klass.typeargs, 1);
    ret = jit_new_map(ctx, kdesc, vdesc, NULL);
  } else if (typo == &tuple_type) {
    ret = jit_new_tuple(ctx, NULL);
  } else {
    expect(typo->alloc != NULL);
    ret = jit_object_alloc(ctx, typo);
  }
  OB_DECREF(type);
  gvector_push_back(&ctx->stack, &ret);
}
