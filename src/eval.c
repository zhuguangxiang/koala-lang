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

static inline uint8 fetch_byte(CallFrame *cf, CodeObject *code)
{
  CodeInfo *codeinfo = code->codeinfo;
  assert(cf->index < codeinfo->size);
  return NEXT_CODE(cf, codeinfo->codes);
}

static inline uint16 fetch_2bytes(CallFrame *cf, CodeObject *code)
{
  CodeInfo *codeinfo = code->codeinfo;
  /* endian? */
  assert(cf->index < codeinfo->size);
  uint8 l = NEXT_CODE(cf, codeinfo->codes);
  uint8 h = NEXT_CODE(cf, codeinfo->codes);
  return (h << 8) + (l << 0);
}

static inline uint8 fetch_opcode(CallFrame *cf, CodeObject *code)
{
  return fetch_byte(cf, code);
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
  Object *v = cf->locvars[index];
  OB_DECREF(v);
  cf->locvars[index] = val;
}

static inline Object *load_field(Object *ob, CodeObject *code, Object *so)
{
  char *name = String_Raw(so);
  if (OB_KLASS(ob) == &Pkg_Klass)
    return Koala_Get_Value((Package *)ob, name);
  else
    return Get_Field(ob, (Klass *)code->owner, name);
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
    size = Vector_Size(&ci->locvec);
  }
  size = sizeof(CallFrame) + size * sizeof(Object *);
  CallFrame *cf = Malloc(size);
  cf->code = code;
  cf->ob = ob;
  cf->args = args;
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
  Mfree(cf);
}

static inline void pop_frame(CallFrame *cf)
{
  KoalaState *ks = cf->ks;
  ks->frame = cf->back;
  free_frame(cf);
}

static void eval_frame(CallFrame *cf)
{
  int loopflag = 1;
  KoalaState *ks = cf->ks;
  Stack *stack = &ks->stack;
  CodeObject *code = (CodeObject *)cf->code;
  Object *consts = code->codeinfo->consts;
  uint8 op;
  int index;
  int argc;
  Object *ob;
  Object *args;
  Object *ret;
  Object *name;

  while (loopflag && (op = fetch_opcode(cf, code), 1)) {
    switch (op) {
    case POP_TOP:
      break;
    case POP_TOPN:
      break;
    case LOAD_CONST:
      index = fetch_2bytes(cf, code);
      ob = index_const(index, consts);
      OB_INCREF(ob);
      PUSH(ob);
      break;
    case LOAD:
      index = fetch_2bytes(cf, code);
      ob = load(cf, index);
      OB_INCREF(ob);
      PUSH(ob);
      break;
    case STORE:
      index = fetch_2bytes(cf, code);
      ob = POP();
      store(cf, index, ob);
      break;
    case LOAD_FIELD:
      index = fetch_2bytes(cf, code);
      name = index_const(index, consts);
      ob = POP();
      ret = load_field(ob, code, name);
      OB_DECREF(ob);
      OB_INCREF(ret);
      PUSH(ret);
      break;
    case STORE_FIELD:

      break;
    case CALL:
      argc = fetch_byte(cf, code);
      index = fetch_2bytes(cf, code);
      name = index_const(index, consts);
      args = Tuple_New(argc);
      //Object *code = get_code(ob, code, name);
      //Koala_RunCode(code, ob, args);
      break;
    case RETURN:
      pop_frame(cf);
      loopflag = 0;
      break;
    case JMP:
      break;
    case NEW:
      break;
    default:
      assert(0);
      break;
    }
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
  OB_INCREF(cf->ob);
  cf->locvars[0] = cf->ob;
  if (cf->args) {
    if (OB_KLASS(cf->args) == &Tuple_Klass) {
      Object *o;
      int size = Tuple_Size(cf->args);
      for (int i = 0; i <= size - 1; i++) {
        o = Tuple_Get(cf->args, i);
        OB_INCREF(o);
        cf->locvars[i + 1] = o;
      }
    } else {
      OB_INCREF(cf->args);
      cf->locvars[1] = cf->args;
    }
  }
}

static inline void run_kfunction(CallFrame *cf)
{
  if (cf->index < 0) {
    check_arguments(cf);
    prepare_kargs(cf);
  }
  eval_frame(cf);
}

static void run_cfunction(CallFrame *cf)
{
  KoalaState *ks = cf->ks;
  Stack *stack = &ks->stack;
  CodeObject *co = (CodeObject *)cf->code;

  check_arguments(cf);
  Object *ret = co->cfunc(cf->ob, cf->args);

  if (ret) {
    /* save result */
    if (OB_KLASS(ret) == &Tuple_Klass) {
      int sz = Tuple_Size(ret);
      Object *ob;
      for (int i = sz - 1; i >= 0; i--) {
        ob = Tuple_Get(ret, i);
        OB_INCREF(ob);
        PUSH(ob);
      }
    } else {
      OB_INCREF(ret);
      PUSH(ret);
    }
    Tuple_Free(ret);
  }
  pop_frame(cf);
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
