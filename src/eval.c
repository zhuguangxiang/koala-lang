/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "koala.h"
#include "codeobject.h"
#include "opcode.h"

LOGGER(0)

typedef struct callframe {
  /* back callframe */
  struct callframe *back;
  /* KoalaState */
  KoalaState *ks;
  /* code */
  Object *code;
  /* object */
  Object *ob;
  /* arguments */
  Object *args;
  /* inst index */
  int index;
  /* local var's count */
  int size;
  /* local var's array */
  Object *locvars[0];
} CallFrame;

#define TOP() Stack_Top(stack)
#define POP() Stack_Pop(stack)
#define PUSH(val) Stack_Push(stack, (val))
#define NEXT_CODE(cf, codes) codes[++(cf)->index]

static inline uint8 fetch_byte(CallFrame *cf, CodeInfo *ci)
{
  assert(cf->index < ci->size);
  return NEXT_CODE(cf, ci->codes);
}

static inline uint16 fetch_2bytes(CallFrame *cf, CodeInfo *ci)
{
  /* endian? */
  assert(cf->index < ci->size);
  uint8 l = NEXT_CODE(cf, ci->codes);
  uint8 h = NEXT_CODE(cf, ci->codes);
  return (h << 8) + (l << 0);
}

static inline uint8 fetch_opcode(CallFrame *cf, CodeInfo *ci)
{
  return fetch_byte(cf, ci);
}

static inline Object *index_const(int index, Object *consts)
{
  return Tuple_Get(consts, index);
}

static inline Object *load(CallFrame *cf, int index)
{
  assert(index < cf->size);
  return cf->locvars[index];
}

static inline void store(CallFrame *cf, int index, Object *val)
{
  assert(val != NULL);
  assert(index < cf->size);
  Object *old = cf->locvars[index];
  cf->locvars[index] = OB_INCREF(val);
  OB_DECREF(old);
}

static inline
Object *load_field(Object *ob, CodeObject *code, Object *so)
{
  char *name = String_Raw(so);
  if (OB_KLASS(ob) == &Pkg_Klass)
    return Pkg_Get_Value(ob, name);
  else
    return Object_Get_Field(ob, (Klass *)code->owner, name);
}

static inline
void store_field(Object *ob, CodeObject *code, Object *so, Object *val)
{
  char *name = String_Raw(so);
  if (OB_KLASS(ob) == &Pkg_Klass)
    return Pkg_Set_Value(ob, name, val);
  else
    return Object_Set_Field(ob, (Klass *)code->owner, name, val);
}

static Object *get_code(Object *ob, Object *so)
{
  char *name = String_Raw(so);
  if (OB_KLASS(ob) == &Pkg_Klass)
    return Pkg_Get_Func(ob, name);
  else
    return Object_Get_Method(ob, NULL, name);
}

#define BINARY_CASE \
  case ADD: \
  case SUB: \
  case MUL: \
  case DIV: \
  case MOD: \
  case POWER: \
  case GT

#define UNARY_CASE \
  case NEG: \
  case NOT: \
  case BNOT

static void push_frame(Object *code, Object *ob, Object *args)
{
  KoalaState *ks = Current_State();

  if (ks->depth >= MAX_FRAME_DEPTH) {
    Log_Error("StackOverflow");
    exit(-1);
  }

  CodeObject *co = (CodeObject *)code;
  int size = 0;
  if (IsKCode(code)) {
    CodeInfo *ci = co->codeinfo;
    /* +1: package or object */
    size = Vector_Size(&ci->locvec) + 1;
  }
  int memsize = sizeof(CallFrame) + size * sizeof(Object *);
  CallFrame *cf = Malloc(memsize);
  cf->code = OB_INCREF(code);
  cf->ob = OB_INCREF(ob);
  cf->args = OB_INCREF(args);
  cf->ks = ks;
  cf->index = -1;
  cf->size = size;
  cf->back = ks->frame;
  ks->frame = cf;
  ks->depth++;
}

static inline void free_frame(CallFrame *cf)
{
  for (int i = 0; i < cf->size; i++)
    OB_DECREF(cf->locvars[i]);
  OB_DECREF(cf->code);
  OB_DECREF(cf->ob);
  OB_DECREF(cf->args);
  Mfree(cf);
}

static inline void pop_frame(CallFrame *cf)
{
  KoalaState *ks = cf->ks;
  ks->frame = cf->back;
  free_frame(cf);
}

static inline Object *current_package(CodeObject *code)
{
  Klass *klazz = (Klass *)code->owner;
  OB_ASSERT_KLASS(klazz, Class_Klass);
  return klazz->owner;
}

static void eval_frame(CallFrame *cf)
{
  int loopflag = 1;
  KoalaState *ks = cf->ks;
  Stack *stack = &ks->stack;
  CodeObject *code = (CodeObject *)cf->code;
  CodeInfo *ci = code->codeinfo;
  Object *consts = ci->consts;
  uint8 op;
  int index;
  int argc;
  Object *fn;
  Object *ob;
  Object *args;
  Object *arg;
  Object *ret;
  Object *name;
  char *path;

  while (loopflag) {
    if ((cf->index + 1) >= ci->size) {
      pop_frame(cf);
      loopflag = 0;
      break;
    }
    op = fetch_opcode(cf, ci);
    switch (op) {
    case POP_TOP:
      break;
    case POP_TOPN:
      break;
    case DUP:
      ob = TOP();
      PUSH(OB_INCREF(ob));
      break;
    case LOAD_CONST:
      index = fetch_2bytes(cf, ci);
      ob = index_const(index, consts);
      PUSH(OB_INCREF(ob));
      break;
    case LOAD_PKG:
      index = fetch_2bytes(cf, ci);
      ob = index_const(index, consts);
      path = String_Raw(ob);
      if (!strcmp(path, "."))
        ob = current_package(code);
      else
        ob = Find_Package(path);
      PUSH(OB_INCREF(ob));
      break;
    case LOAD:
      index = fetch_2bytes(cf, ci);
      ob = load(cf, index);
      PUSH(OB_INCREF(ob));
      break;
    case LOAD_0:
      ob = load(cf, 0);
      PUSH(OB_INCREF(ob));
      break;
    case LOAD_1:
      ob = load(cf, 1);
      PUSH(OB_INCREF(ob));
      break;
    case LOAD_2:
      ob = load(cf, 2);
      PUSH(OB_INCREF(ob));
      break;
    case LOAD_3:
      ob = load(cf, 3);
      PUSH(OB_INCREF(ob));
      break;
    case STORE:
      index = fetch_2bytes(cf, ci);
      ob = POP();
      store(cf, index, ob);
      OB_DECREF(ob);
      break;
    case STORE_0:
      ob = POP();
      store(cf, 0, ob);
      OB_DECREF(ob);
      break;
    case STORE_1:
      ob = POP();
      store(cf, 1, ob);
      OB_DECREF(ob);
      break;
    case STORE_2:
      ob = POP();
      store(cf, 2, ob);
      OB_DECREF(ob);
      break;
    case STORE_3:
      ob = POP();
      store(cf, 3, ob);
      OB_DECREF(ob);
      break;
    case GET_ATTR:
      index = fetch_2bytes(cf, ci);
      name = index_const(index, consts);
      ob = POP();
      ret = load_field(ob, code, name);
      OB_DECREF(ob);
      PUSH(OB_INCREF(ret));
      break;
    case SET_ATTR:
      index = fetch_2bytes(cf, ci);
      name = index_const(index, consts);
      ob = POP();
      arg = POP();
      store_field(ob, code, name, arg);
      OB_DECREF(ob);
      OB_DECREF(arg);
      break;
    case ADD:
      ob = POP();
      OB_DECREF(ob);
      ob = POP();
      OB_DECREF(ob);
      ob = Integer_New(3);
      PUSH(OB_INCREF(ob));
      OB_DECREF(ob);
      break;
    case CALL:
      index = fetch_2bytes(cf, ci);
      argc = fetch_byte(cf, ci);
      name = index_const(index, consts);
      ob = POP();
      if (argc == 1) {
        args = POP();
      } else if (argc > 1) {
        args = Tuple_New(argc);
        while (argc-- > 0) {
          arg = POP();
          Tuple_Append(args, arg);
          OB_DECREF(arg);
        }
      } else {
        assert(argc == 0);
        args = NULL;
      }
      fn = get_code(ob, name);
      push_frame(fn, ob, args);
      OB_DECREF(ob);
      OB_DECREF(args);
      loopflag = 0;
      break;
    case RETURN:
      pop_frame(cf);
      loopflag = 0;
      break;
    case NEW_ENUM:
      index = fetch_2bytes(cf, ci);
      name = index_const(index, consts);
      argc = fetch_byte(cf, ci);
      ob = POP();
      if (argc == 1) {
        args = POP();
      } else if (argc > 1) {
        args = Tuple_New(argc);
        while (argc-- > 0) {
          arg = POP();
          Tuple_Append(args, arg);
          OB_DECREF(arg);
        }
      } else {
        assert(argc == 0);
        args = NULL;
      }
      ret = Enum_New(ob, name, args);
      OB_DECREF(ob);
      OB_DECREF(args);
      PUSH(OB_INCREF(ret));
      OB_DECREF(ret);
      break;
    default:
      assert(0);
      break;
    }
  }
}

static void show_stackframe(CallFrame *cf)
{
  CodeObject *code;
  Object *owner;
  char *prefix;
  CallFrame *f = cf;
  while (f != NULL) {
    code = (CodeObject *)f->code;
    owner = code->owner;
    if (OB_KLASS(owner) == &Pkg_Klass) {
      prefix = Pkg_Name(owner);
    }
    Log_Printf("%s.%s()\n", prefix, code->name);
    f = f->back;
  }
}

static void check_arguments(CallFrame *cf)
{
  int size = Tuple_Size(cf->args);
  int argc = Code_Get_Argc(cf->code);
  Log_Debug("call argc: %d, proto argc: %d", size, argc);
  if (size < argc) {
    Log_Error("call argc %d < proto argc %d", size, argc);
    exit(-1);
  }
}

static void prepare_kargs(CallFrame *cf)
{
  cf->locvars[0] = OB_INCREF(cf->ob);
  if (cf->args) {
    if (OB_KLASS(cf->args) == &Tuple_Klass) {
      Object *o;
      int size = Tuple_Size(cf->args);
      assert(cf->size == size + 1);
      for (int i = 0; i <= size - 1; i++) {
        o = Tuple_Get(cf->args, i);
        cf->locvars[i + 1] = OB_INCREF(o);
      }
    } else {
      assert(cf->size == 2);
      cf->locvars[1] = OB_INCREF(cf->args);
    }
  }
}

static inline void run_kfunction(CallFrame *cf)
{
  if (cf->index < 0) {
    //check_arguments(cf);
    prepare_kargs(cf);
  }
  eval_frame(cf);
}

static void run_cfunction(CallFrame *cf)
{
  KoalaState *ks = cf->ks;
  Stack *stack = &ks->stack;
  CodeObject *co = (CodeObject *)cf->code;

  //show_stackframe(cf);

  //check_arguments(cf);
  Object *ret = co->cfunc(cf->ob, cf->args);

  if (ret) {
    /* save result */
    if (OB_KLASS(ret) == &Tuple_Klass) {
      int sz = Tuple_Size(ret);
      Object *ob;
      for (int i = sz - 1; i >= 0; i--) {
        ob = Tuple_Get(ret, i);
        PUSH(OB_INCREF(ob));
      }
    } else {
      PUSH(OB_INCREF(ret));
    }
    OB_DECREF(ret);
  }
  pop_frame(cf);
}

void loop_frame(void)
{

}

void Koala_RunCode(Object *code, Object *ob, Object *args)
{
  CallFrame *cf;
  KoalaState *ks = Current_State();
  push_frame(code, ob, args);
  while ((cf = ks->frame) != NULL) {
    if (IsKCode(cf->code)) {
      run_kfunction(cf);
    } else if (IsCFunc(cf->code)) {
      run_cfunction(cf);
    } else {
      Log_Error("Invalid function type");
      exit(-1);
    }
  }
}

void Koala_Call(Object *code, Object *ob, Object *args)
{
  push_frame(code, ob, args);
}
