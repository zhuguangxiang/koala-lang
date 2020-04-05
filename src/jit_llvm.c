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
#include "opcode.h"

#define WHEEL_NEW_TUPLE     0
#define WHEEL_NEW_ARRAY     1
#define WHEEL_NEW_MAP       2
#define WHEEL_NEW_OBJECT    3
#define WHEEL_KOALA_CALL    4
#define WHEEL_GET_VALUE     5
#define WHEEL_SET_VALUE     6
#define WHEEL_SUBSCR_LOAD   7
#define WHEEL_SUBSCR_STORE  8
#define WHEEL_INCREF_OBJ    9
#define WHEEL_DECREF_OBJ    10
#define WHEEL_ENTER_FUNC    11
#define WHEEL_EXIT_FUNC     12
#define MAX_WHEEL_NR        13

static JitFunction wheel_funcs[MAX_WHEEL_NR];
static LLVMExecutionEngineRef llengine;
static LLVMModuleRef llmod;
static HashMap funcmap;

typedef struct funcnode {
  HashMapEntry entry;
  char *name;
  JitFunction *func;
} FuncNode;

static int funcnode_equal(void *p1, void *p2)
{
  FuncNode *n1 = p1;
  FuncNode *n2 = p2;
  return !strcmp(n1->name, n2->name);
}

void jit_free_function(JitFunction *f);

static void _funcnode_free_(void *p, void *q)
{
  FuncNode *n = p;
  jit_free_function(n->func);
  kfree(n);
}

void init_func_map(void)
{
  hashmap_init(&funcmap, funcnode_equal);
}

void fini_func_map(void)
{
  hashmap_fini(&funcmap, _funcnode_free_, NULL);
}

int jit_add_function(JitFunction *func)
{
  FuncNode *node = kmalloc(sizeof(FuncNode));
  node->name = func->name;
  node->func = func;
  hashmap_entry_init(node, strhash(node->name));
  return hashmap_add(&funcmap, node);
}

JitFunction *jit_find_function(char *name)
{
  FuncNode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  FuncNode *node = hashmap_get(&funcmap, &key);
  return node ? node->func : NULL;
}

#define JIT_MODULE_NAME "__koala_jitter__"

static LLVMModuleRef llvm_module(void)
{
  if (llmod == NULL) {
    llmod = LLVMModuleCreateWithName(JIT_MODULE_NAME);
  }
  return llmod;
}

static LLVMExecutionEngineRef llvm_engine(LLVMModuleRef mod)
{
  if (llengine == NULL) {
    char *error;
    LLVMExecutionEngineRef engine;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    //LLVMCreateExecutionEngineForModule(&engine, mod, &error);
    LLVMCreateJITCompilerForModule(&engine, mod, 2, &error);
    llengine = engine;
  } else {
    LLVMAddModule(llengine, mod);
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

void __obj_decref__(void *self)
{
  Object *ob = self;
  printf("decref of '%s', count: %d\n", OB_TYPE_NAME(ob), ob->ob_refcnt);
  OB_DECREF(ob);
}

void __obj_incref__(void *self)
{
  Object *ob = self;
  printf("incref of '%s', count: %d\n", OB_TYPE_NAME(ob), ob->ob_refcnt);
  OB_INCREF(ob);
}

void __enterfunc__(void *args[], int32_t argc)
{

}

void __exitfunc__(void *args[], int32_t argc)
{
  Object *ob;
  for (int i = 0; i < argc; ++i) {
    ob = args[i];
    printf("decref of '%s', count: %d\n", OB_TYPE_NAME(ob), ob->ob_refcnt);
    OB_DECREF(ob);
  }
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

static int64_t __obj_unbox__(Object *ob)
{
  int64_t val;
  if (integer_check(ob)) {
    val = integer_asint(ob);
  } else if (byte_check(ob)) {
    val = byte_asint(ob);
  } else if (bool_check(ob)) {
    val = bool_istrue(ob);
  } else if (float_check(ob)) {
    val = (int64_t)float_asflt(ob);
  } else if (char_check(ob)) {
    val = char_asch(ob);
  } else if (string_check(ob)) {
    val = (int64_t)string_asstr(ob);
  } else {
    val = (int64_t)(intptr_t)OB_INCREF(ob);
  }
  return val;
}

/* int64_t __subscr_load__(Object *ob, int64_t index); */
int64_t __subscr_load__(Object *ob, int64_t index)
{
  func_t fn = OB_MAP_FUNC(ob, getitem);
  Object *value = integer_new(index);
  Object *z;
  if (fn != NULL) {
    z = fn(ob, value);
  } else {
    z = object_call(ob, opcode_map(OP_SUBSCR_LOAD), value);
  }
  OB_DECREF(value);
  int64_t res = __obj_unbox__(z);
  OB_DECREF(z);
  return res;
}

/* void __subscr_store__(Object *ob, int64_t index, void *val, int kind); */
void __subscr_store__(Object *ob, int64_t index, void *val, int kind)
{
  func_t fn = OB_MAP_FUNC(ob, setitem);
  Object *value = integer_new(index);
  Object *arg = __obj_box__(kind, val);
  Object *tuple = tuple_new(2);
  tuple_set(tuple, 0, value);
  tuple_set(tuple, 1, arg);
  if (fn != NULL) {
    fn(ob, tuple);
  } else {
    object_call(ob, opcode_map(OP_SUBSCR_STORE), tuple);
  }
  OB_DECREF(tuple);
  OB_DECREF(value);
  OB_DECREF(arg);
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

int64_t koala_call(Object *func, Object *ob, void *args[], int32_t argc)
{
  TypeDesc *desc = ((MethodObject *)func)->desc;
  Vector *argtypes = desc->proto.args;
  JitType type;
  Object *tuple;
  if (argc <= 0) {
    tuple = NULL;
  } else if (argc == 1) {
    type = jit_type(vector_get(argtypes, 0));
    tuple = __obj_box__(type.kind, args[0]);
  } else {
    tuple = tuple_new(argc);
    Object *arg;
    for (int i = 0; i < argc; i++) {
      type = jit_type(vector_get(argtypes, i));
      arg = __obj_box__(type.kind, args[i]);
      tuple_set(tuple, i, arg);
      OB_DECREF(arg);
    }
  }
  Object *res = method_call(func, ob, tuple);
  int64_t value = __obj_unbox__(res);
  OB_DECREF(res);
  return value;
}

int64_t __get_value__(Object *self, char *name, TypeObject *type)
{
  Object *ob = object_getvalue(self, name, type);
  int64_t val = __obj_unbox__(ob);
  OB_DECREF(ob);
  return val;
}

void __set_value__(Object *self, char *name, void *raw, int kind,
                  TypeObject *type)
{
  Object *ob = object_lookup(self, name, type);
  if (ob == NULL) {
    error("object of '%s' has no field '%s'", OB_TYPE_NAME(self), name);
    return;
  }

  if (!field_check(ob)) {
    error("'%s' is not setable", name);
    OB_DECREF(ob);
    return;
  }

  Object *val = __obj_box__(kind, raw);
  int res = field_set(ob, self, val);
  OB_DECREF(ob);
  OB_DECREF(val);
}

/* int64_t __subscr_load__(Object *ob, int64_t index); */
static void init_subscr_load(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_SUBSCR_LOAD];
  JitType params[] = {
    JitPtrType,
    JitIntType,
  };
  JitType ret = JitIntType;
  jit_init_cfunc(f, "__subscr_load__", ret, params, 2);
}

/* void __subscr_store__(Object *ob, int64_t index, void *val, int kind); */
static void init_subscr_store(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_SUBSCR_STORE];
  JitType params[] = {
    JitPtrType,
    JitIntType,
    JitPtrType,
    JitIntType,
  };
  JitType ret = JitVoidType;
  jit_init_cfunc(f, "__subscr_store__", ret, params, 4);
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

/* int64_t koala_call(Object *func, Object *ob, void *args[], int32_t argc); */
static void init_koala_call(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_KOALA_CALL];
  JitType params[] = {
    JitPtrType,
    JitPtrType,
    JitArgsType,
    JitNrType,
  };
  JitType ret = JitIntType;
  jit_init_cfunc(f, "koala_call", ret, params, 4);
}

/*
  int64_t __get_value__(Object *self, char *name, TypeObject *type);
 */
static void init_get_value(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_GET_VALUE];
  JitType params[] = {
    JitPtrType,
    JitPtrType,
    JitPtrType,
  };
  JitType ret = JitIntType;
  jit_init_cfunc(f, "__get_value__", ret, params, 3);
}

/*
  void __set_value__(Object *self, char *name, void *val, int kind,
                    TypeObject *type);
 */
static void init_set_value(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_SET_VALUE];
  JitType params[] = {
    JitPtrType,
    JitPtrType,
    JitPtrType,
    JitIntType,
    JitPtrType,
  };
  JitType ret = JitVoidType;
  jit_init_cfunc(f, "__set_value__", ret, params, 5);
}

/* void __obj_incref__(void *self) */
static void init_inc_obj(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_INCREF_OBJ];
  JitType params[] = {
    JitPtrType,
  };
  JitType ret = JitVoidType;
  jit_init_cfunc(f, "__obj_incref__", ret, params, 1);
}

/* void __obj_decref__(void *self) */
static void init_dec_obj(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_DECREF_OBJ];
  JitType params[] = {
    JitPtrType,
  };
  JitType ret = JitVoidType;
  jit_init_cfunc(f, "__obj_decref__", ret, params, 1);
}

/* void __enterfunc__(void *args[], int32_t argc); */
static void init_enter_func(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_ENTER_FUNC];
  JitType params[] = {
    JitArgsType,
    JitNrType,
  };
  JitType ret = JitVoidType;
  jit_init_cfunc(f, "__enterfunc__", ret, params, 2);
}

/* void __exitfunc__(void *args[], int32_t argc); */
static void init_exit_func(void)
{
  JitFunction *f = &wheel_funcs[WHEEL_EXIT_FUNC];
  JitType params[] = {
    JitArgsType,
    JitNrType,
  };
  JitType ret = JitVoidType;
  jit_init_cfunc(f, "__exitfunc__", ret, params, 2);
}

static void init_wheel_funcs(void)
{
  init_new_tuple();
  init_new_array();
  init_new_map();
  init_new_object();
  init_koala_call();
  init_get_value();
  init_set_value();
  init_subscr_load();
  init_subscr_store();
  init_inc_obj();
  init_dec_obj();
  init_enter_func();
  init_exit_func();
}

static void fini_wheel_funcs(void)
{
  JitFunction *f;
  for (int i = 0; i < MAX_WHEEL_NR; ++i) {
    f = &wheel_funcs[i];
    gvector_fini(&f->argtypes);
  }
}

/*--------------------------------------------------------------------------*/

LLVMPassManagerRef llpass;

void init_jit_llvm(void)
{
  init_types();
  init_wheel_funcs();
  init_func_map();
  /*
  llpass = LLVMCreateFunctionPassManagerForModule(llvm_module());
  // This pass should eliminate all the load and store instructions
  LLVMAddPromoteMemoryToRegisterPass(llpass);

  // Add some optimization passes
  LLVMAddScalarReplAggregatesPass(llpass);
  LLVMAddLICMPass(llpass);
  LLVMAddAggressiveDCEPass(llpass);
  LLVMAddCFGSimplificationPass(llpass);
  LLVMAddInstructionCombiningPass(llpass);
  LLVMAddTailCallEliminationPass(llpass);
  LLVMAddBasicAliasAnalysisPass(llpass);
  LLVMAddDeadStoreEliminationPass(llpass);
  LLVMAddMergedLoadStoreMotionPass(llpass);
  LLVMAddLowerAtomicPass(llpass);

  // Run the pass
  //LLVMInitializeFunctionPassManager(gallivm->passmgr);
  //LLVMRunFunctionPassManager(gallivm->passmgr, ctx->main_fn);
  //LLVMFinalizeFunctionPassManager(gallivm->passmgr);

  //LLVMDisposeBuilder(gallivm->builder);
  //LLVMDisposePassManager(gallivm->passmgr);

  int res = LLVMInitializeFunctionPassManager(llpass);
  printf("LLVMInitializeFunctionPassManager:%d\n", res);
  */
}

void fini_jit_llvm(void)
{
  fini_wheel_funcs();
  fini_func_map();
  //LLVMFinalizeFunctionPassManager(llpass);
  //LLVMDisposePassManager(llpass);

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
  value.raw = ival;
  value.type = JitIntType;
  return value;
}

static JitValue jit_const_byte(int8_t bval)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitByteType.lltype, bval, 0);
  value.raw = bval;
  value.type = JitByteType;
  return value;
}

static JitValue jit_const_bool(int8_t zval)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitBoolType.lltype, zval, 0);
  value.raw = zval;
  value.type = JitBoolType;
  return value;
}

static JitValue jit_const_float(double fval)
{
  JitValue value;
  value.llvalue = LLVMConstReal(JitFloatType.lltype, fval);
  value.raw = (int64_t)fval;
  value.type = JitFloatType;
  return value;
}

static JitValue jit_const_char(int32_t ival)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitCharType.lltype, ival, 1);
  value.raw = ival;
  value.type = JitCharType;
  return value;
}

static JitValue jit_const_str(void *ptr)
{
  JitValue value;
  LLVMValueRef llvalue = LLVMConstInt(LLVMIntPtrType2(), (intptr_t)ptr, 0);
  value.llvalue = LLVMConstIntToPtr(llvalue, JitStrType.lltype);
  value.raw = (int64_t)ptr;
  value.type = JitStrType;
  return value;
}

static JitValue jit_const_ptr(void *ptr)
{
  JitValue value;
  LLVMValueRef llvalue = LLVMConstInt(LLVMIntPtrType2(), (intptr_t)ptr, 0);
  value.llvalue = LLVMConstIntToPtr(llvalue, JitPtrType.lltype);
  value.raw = (int64_t)ptr;
  value.type = JitPtrType;
  return value;
}

static JitValue jit_const_num(int32_t ival)
{
  JitValue value;
  value.llvalue = LLVMConstInt(JitNrType.lltype, ival, 1);
  value.raw = ival;
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
  var->name = name;
  return var;
}

void jit_free_variable(JitVariable *var)
{
  kfree(var);
}

static inline int jit_is_object(JitValue *value)
{
  JitType *type = &value->type;
  int isobj = type->kind == 'p' || type->kind == 's';
  return isobj && value->llvalue != NULL;
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
    strbuf_append_char(&sbuf, '$');
    if (co->type != NULL) {
      strbuf_append(&sbuf, co->type->name);
      strbuf_append_char(&sbuf, '$');
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

  f->name = function_name(co);
  f->llproto = LLVMFunctionType(ret, params, nargs, 0);
  f->llfunc  = LLVMAddFunction(llvm_module(), f->name, f->llproto);
  jit_add_function(f);
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
  ctx->nrargs = args;
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

  //jit_free_function(ctx->func);

}

void *jit_emit_code(JitContext *ctx)
{
  char *name = function_name(ctx->co);
  LLVMModuleRef mod = llvm_module();
  JitFunction *f = jit_find_function(name);
  //int res = LLVMRunFunctionPassManager(llpass, f->llfunc);
  //printf("jit_optimize:%d\n", res);

  void *mcptr = (void *)LLVMGetFunctionAddress(llvm_engine(mod), name);
  LLVMRemoveModule(llengine, mod, &mod, NULL);
  LLVMDeleteFunction(f->llfunc);
  LLVMValueRef llfunc = LLVMAddFunction(llvm_module(), name, f->llproto);
  LLVMSetLinkage(llfunc, LLVMExternalLinkage);
  LLVMAddGlobalMapping(llengine, llfunc, mcptr);
  f->llfunc = llfunc;
  return mcptr;
}

void jit_optimize(void)
{
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
  JitValue value = {llvalue, 0, f->rettype};
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

/* void __obj_incref__(void *self) */
static void jit_incref(JitContext *ctx, JitValue *value)
{
  JitFunction *f = &wheel_funcs[WHEEL_INCREF_OBJ];
  LLVMValueRef params[] = {
    value->llvalue
  };
  jit_build_call(ctx, f, params, 1);
}

/* void __obj_decref__(void *self) */
static void jit_decref(JitContext *ctx, JitValue *value)
{
  JitFunction *f = &wheel_funcs[WHEEL_DECREF_OBJ];
  LLVMValueRef params[] = {
    value->llvalue
  };
  jit_build_call(ctx, f, params, 1);
}

void jit_OP_POP_TOP(JitContext *ctx)
{
  JitValue value;
  gvector_pop_back(&ctx->stack, &value);
  if (jit_is_object(&value)) {
    jit_decref(ctx, &value);
  }
}

void jit_OP_LOAD_CONST(JitContext *ctx, int index)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);
  JitValue value = jit_const(x);
  if (jit_is_object(&value)) {
    jit_incref(ctx, &value);
  }
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
  if (jit_is_object((JitValue *)var)) {
    // decrease object refcnt
    jit_decref(ctx, (JitValue *)var);
  }
  var->llvalue = value.llvalue;
}

/*
  int64_t __set_value__(Object *self, char *name, TypeObject *type);
 */
void jit_OP_GET_VALUE(JitContext *ctx, int index)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);

  JitValue y;
  gvector_pop_back(&ctx->stack, &y);
  JitFunction *f = &wheel_funcs[WHEEL_GET_VALUE];
  LLVMValueRef params[] = {
    y.llvalue,
    jit_const_ptr(string_asstr(x)).llvalue,
    jit_const_ptr(co->type).llvalue,
  };
  JitValue value = jit_build_call(ctx, f, params, 3);
  gvector_push_back(&ctx->stack, &value);
  OB_DECREF(x);
}

/*
  void __set_value__(Object *self, char *name, void *val, int kind,
                    TypeObject *type);
 */
void jit_OP_SET_VALUE(JitContext *ctx, int index)
{
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);

  JitValue y, z;
  gvector_pop_back(&ctx->stack, &y);
  gvector_pop_back(&ctx->stack, &z);

  JitFunction *f = &wheel_funcs[WHEEL_SET_VALUE];
  LLVMValueRef params[] = {
    y.llvalue,
    jit_const_ptr(string_asstr(x)).llvalue,
    jit_value_to_voidptr(ctx, &z),
    jit_const_int(z.type.kind).llvalue,
    jit_const_ptr(co->type).llvalue,
  };
  jit_build_call(ctx, f, params, 5);
  OB_DECREF(x);
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

/* int64_t koala_call(Object *func, Object *ob, void *args[], int32_t argc); */
static JitValue jit_koala_call(JitContext *ctx, Object *self,
                              char *funcname, GVector *args)
{
  Object *meth = object_lookup(self, funcname, NULL);
  JitFunction *f = &wheel_funcs[WHEEL_KOALA_CALL];
  int size = gvector_size(args);
  LLVMValueRef llargs = jit_voidptr_array(ctx, args);
  LLVMValueRef params[] = {
    jit_const_ptr(meth).llvalue,
    jit_const_ptr(self).llvalue,
    llargs,
    jit_const_num(size).llvalue,
  };
  JitValue res = jit_build_call(ctx, f, params, 4);
  jit_free_array(ctx, llargs);
  return res;
}

void jit_OP_CALL(JitContext *ctx, int index, int count)
{
  LLVMBuilderRef builder = ctx->builder;
  CodeObject *co = ctx->co;
  Object *consts = co->consts;
  Object *x = tuple_get(consts, index);

  JitValue value;
  gvector_pop_back(&ctx->stack, &value);

  char *funcname;
  Object *self = (Object *)value.raw;
  STRBUF(sbuf);
  if (module_check(self)) {
    strbuf_append(&sbuf, MODULE_PATH(self));
  } else {
    TypeObject *type = OB_TYPE(self);
    strbuf_append(&sbuf, MODULE_PATH(type->owner));
    strbuf_append_char(&sbuf, '$');
    strbuf_append(&sbuf, type->name);
  }
  strbuf_append_char(&sbuf, '$');
  strbuf_append(&sbuf, string_asstr(x));
  funcname = atom(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  JitFunction *func = jit_find_function(funcname);

  if (func != NULL) {
    LLVMValueRef params[count];
    for (int i = 0; i < count; ++i) {
      gvector_pop_back(&ctx->stack, &value);
      params[i] = value.llvalue;
    }
    value = jit_build_call(ctx, func, params, count);
    gvector_push_back(&ctx->stack, &value);
  } else {
    GVector vec;
    gvector_init(&vec, 16, sizeof(JitValue));
    for (int i = 0; i < count; ++i) {
      gvector_pop_back(&ctx->stack, &value);
      gvector_push_back(&vec, &value);
    }
    value = jit_koala_call(ctx, self, string_asstr(x), &vec);
    gvector_push_back(&ctx->stack, &value);
    gvector_fini(&vec);
  }
  OB_DECREF(x);
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

/* int64_t __subscr_load__(Object *ob, int64_t index); */
void jit_OP_SUBSCR_LOAD(JitContext *ctx)
{
  JitValue x, y;
  gvector_pop_back(&ctx->stack, &x);
  gvector_pop_back(&ctx->stack, &y);
  JitFunction *f = &wheel_funcs[WHEEL_SUBSCR_LOAD];
  LLVMValueRef params[] = {
    y.llvalue,
    x.llvalue,
  };
  JitValue value = jit_build_call(ctx, f, params, 2);
  gvector_push_back(&ctx->stack, &value);
}

/* void __subscr_store__(Object *ob, int64_t index, void *val, int kind); */
void jit_OP_SUBSCR_STORE(JitContext *ctx)
{
  JitValue x, y, z;
  gvector_pop_back(&ctx->stack, &y);
  gvector_pop_back(&ctx->stack, &x);
  gvector_pop_back(&ctx->stack, &z);
  JitFunction *f = &wheel_funcs[WHEEL_SUBSCR_STORE];
  LLVMValueRef params[] = {
    x.llvalue,
    y.llvalue,
    jit_value_to_voidptr(ctx, &z),
    jit_const_int(z.type.kind).llvalue,
  };
  jit_build_call(ctx, f, params, 4);
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

void jit_OP_LOAD_GLOBAL(JitContext *ctx)
{
  CodeObject *co = ctx->co;
  Object *mod = co->module;
  JitValue value = jit_const_ptr(mod);
  gvector_push_back(&ctx->stack, &value);
}

void jit_enter_func(JitContext *ctx)
{
  GVector vec;
  gvector_init(&vec, 16, sizeof(JitValue));

  JitValue value;
  JitVariable *var;
  for (int i = 0; i < ctx->nrargs; ++i) {
    var = vector_get(&ctx->locals, i + 1);
    if (jit_is_object((JitValue *)var)) {
      printf("arg:'%s' is object\n", var->name);
      value = *(JitValue *)var;
      gvector_push_back(&vec, &value);
    }
  }

  if (gvector_size(&vec) > 0) {
    JitFunction *f = &wheel_funcs[WHEEL_ENTER_FUNC];
    LLVMValueRef llargs = jit_voidptr_array(ctx, &vec);
    LLVMValueRef params[] = {
      llargs,
      jit_const_num(gvector_size(&vec)).llvalue,
    };
    JitValue res = jit_build_call(ctx, f, params, 2);
    jit_free_array(ctx, llargs);
  }

  gvector_fini(&vec);
}

void jit_exit_func(JitContext *ctx)
{
  GVector vec;
  gvector_init(&vec, 16, sizeof(JitValue));

  JitValue value;
  JitVariable *var;
  int locals = vector_size(&ctx->locals);
  for (int i = ctx->nrargs + 1; i < locals; ++i) {
    var = vector_get(&ctx->locals, i);
    if (jit_is_object((JitValue *)var)) {
      printf("[FUNC-EXIT]: loc:'%s' is object\n", var->name);
      value = *(JitValue *)var;
      gvector_push_back(&vec, &value);
    }
  }

  if (gvector_size(&vec) > 0) {
    JitFunction *f = &wheel_funcs[WHEEL_EXIT_FUNC];
    LLVMValueRef llargs = jit_voidptr_array(ctx, &vec);
    LLVMValueRef params[] = {
      llargs,
      jit_const_num(gvector_size(&vec)).llvalue,
    };
    JitValue res = jit_build_call(ctx, f, params, 2);
    jit_free_array(ctx, llargs);
  }

  gvector_fini(&vec);
}
