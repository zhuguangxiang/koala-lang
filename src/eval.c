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

#include <pthread.h>
#include "eval.h"
#include "codeobject.h"
#include "intobject.h"
#include "floatobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "arrayobject.h"
#include "rangeobject.h"
#include "mapobject.h"
#include "moduleobject.h"
#include "iomodule.h"
#include "opcode.h"
#include "log.h"

typedef struct frame {
  /* back call frame */
  struct frame *back;
  /* KoalaState */
  KoalaState *ks;
  /* code */
  Object *code;
  /* code index */
  int index;
  /* local variables' size */
  int size;
  /* local variables's array */
  Object *locvars[1];
} Frame;

static Frame *new_frame(KoalaState *ks, Object *code, int locals)
{
  expect(ks->depth < MAX_FRAME_DEPTH);
  Frame *f = kmalloc(sizeof(Frame) + locals * sizeof(Object *));
  f->code = OB_INCREF(code);
  f->size = locals + 1;
  f->index = -1;
  f->back = ks->frame;
  f->ks = ks;
  ks->frame = f;
  ++ks->depth;
  return f;
}

static void free_frame(Frame *f)
{
  for (int i = 0; i < f->size; i++) {
    OB_DECREF(f->locvars[i]);
  }
  OB_DECREF(f->code);
  kfree(f);
}

static void prepare_args(Frame *f, Object *ob, Object *args)
{
  f->locvars[0] = OB_INCREF(ob);
  if (args != NULL) {
    if (Tuple_Check(args)) {
      Object *v;
      int size = Tuple_Size(args);
      for (int i = 0; i < size; i++) {
        v = Tuple_Get(args, i);
        f->locvars[i + 1] = OB_INCREF(v);
        OB_DECREF(v);
      }
    } else {
      f->locvars[1] = OB_INCREF(args);
    }
  }
}

/* global KoalaState list */
static struct list_head kslist;

#define TOP() ({ \
  expect(top < base + MAX_STACK_SIZE); \
  *top; \
})

#define POP() ({ \
  expect(top >= base); \
  *top--; \
})

#define PUSH(v) ({ \
  expect(top < base + MAX_STACK_SIZE - 1); \
  *++top = v; \
})

#define NEXT_OP() ({ \
  expect(f->index < co->size); \
  co->codes[++f->index]; \
})

#define NEXT_BYTE() ({ \
  expect(f->index < co->size); \
  co->codes[++f->index]; \
})

#define NEXT_2BYTES() ({ \
  uint8_t l = NEXT_BYTE(); \
  uint8_t h = NEXT_BYTE(); \
  ((h << 8) + l); \
})

#define GETLOCAL(i) ({ \
  expect(i < f->size); \
  f->locvars[i]; \
})

#define SETLOCAL(i, v) ({ \
  expect(i < f->size); \
  OB_DECREF(f->locvars[i]); \
  f->locvars[i] = v; \
})

#define call_op_func(z, x, op, y) \
do { \
  if (fn != NULL) { \
    z = fn(x, y); \
  } else { \
    z = Object_Call(x, opcode_map(op), y); \
  } \
} while (0)

static int logic_true(Object *ob)
{
  if (ob == NULL)
    return 0;
  if (Bool_Check(ob)) {
    return Bool_IsTrue(ob) ? 1 : 0;
  } else if (integer_check(ob)) {
    return integer_asint(ob) ? 1 : 0;
  } else if (Byte_Check(ob)) {
    return Byte_AsInt(ob) ? 1 : 0;
  } else if (string_check(ob)) {
    return string_isempty(ob) ? 0 : 1;
  } else if (Char_Check(ob)) {
    return Char_AsChar(ob) ? 1 : 0;
  } else {
    panic("Not Implemented");
    return 0;
  }
}

static int typecheck(Object *ob, Object *type)
{
  if (!descob_check(type))
    panic("typecheck: para is not DescType");
  TypeDesc *desc = ((DescObject *)type)->desc;
  if (integer_check(ob) && desc_isint(desc))
    return 1;
  if (string_check(ob) && desc_isstr(desc))
    return 1;
  if (Bool_Check(ob) && desc_isbool(desc))
    return 1;
  TypeObject *typeob = OB_TYPE(ob);
  if (desc_check(typeob->desc, desc)) {
    #if 0
    if (array_check(ob)) {
      ArrayObject *arr = (ArrayObject *)ob;
      return desc_check(arr->desc, vector_get(desc->types->vec, 0));
    }
    #endif
    return 1;
  }
  return 0;
}

static Object *new_object(TypeDesc *desc)
{
  Object *ret;
  Object *ob = Module_Load(desc->klass.path);
  expect(ob != NULL);
  ob = Module_Lookup(ob, desc->klass.type);
  expect(ob != NULL);

  TypeObject *type = (TypeObject *)ob;
  if (type == &array_type) {
    TypeDesc *subdesc = vector_get(desc->types, 0);
    ret = array_new(subdesc);
  } else if (type == &map_type) {
    TypeDesc *kdesc = vector_get(desc->types, 0);
    TypeDesc *vdesc = vector_get(desc->types, 1);
    ret= map_new(kdesc, vdesc);
  } else if (type == &tuple_type) {
    ret = Tuple_New(0);
  } else {
    ret = NULL;
  }
  return ret;
}

Object *Koala_EvalFrame(Frame *f)
{
  int loopflag = 1;
  KoalaState *ks = f->ks;
  Object **base = ks->stack;
  Object **top = base + ks->top;
  CodeObject *co = (CodeObject *)f->code;
  Object *consts = co->consts;
  uint8_t op;
  int oparg;
  Object *x, *y, *z, *v, *w;
  TypeDesc *desc, *xdesc, *ydesc;
  func_t fn;
  int i;

  while (loopflag) {
    if ((f->index + 1) >= co->size) {
      loopflag = 0;
      break;
    }
    op = NEXT_OP();
    switch (op) {
    case OP_POP_TOP: {
      x = POP();
      OB_DECREF(x);
      break;
    }
    case OP_DUP: {
      x = TOP();
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_CONST_BYTE: {
      oparg = NEXT_BYTE();
      x = integer_new(oparg);
      PUSH(x);
      break;
    }
    case OP_CONST_SHORT: {
      oparg = NEXT_2BYTES();
      x = integer_new(oparg);
      PUSH(x);
      break;
    }
    case OP_LOAD_CONST: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = dup_const_object(x);
      OB_DECREF(x);
      PUSH(y);
      break;
    }
    case OP_LOAD_MODULE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = Module_Load(string_asstr(x));
      PUSH(y);
      OB_DECREF(x);
      break;
    }
    case OP_LOAD: {
      oparg = NEXT_BYTE();
      x = GETLOCAL(oparg);
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_LOAD_0: {
      x = GETLOCAL(0);
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_LOAD_1: {
      x = GETLOCAL(1);
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_LOAD_2: {
      x = GETLOCAL(2);
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_LOAD_3: {
      x = GETLOCAL(3);
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_STORE: {
      oparg = NEXT_BYTE();
      x = POP();
      SETLOCAL(oparg, x);
      break;
    }
    case OP_STORE_0: {
      x = POP();
      SETLOCAL(0, x);
      break;
    }
    case OP_STORE_1: {
      x = POP();
      SETLOCAL(1, x);
      break;
    }
    case OP_STORE_2: {
      x = POP();
      SETLOCAL(2, x);
      break;
    }
    case OP_STORE_3: {
      x = POP();
      SETLOCAL(3, x);
      break;
    }
    case OP_GET_OBJECT: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = POP();
      z = Object_Lookup(y, string_asstr(x));
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_GET_VALUE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = POP();
      z = Object_GetValue(y, string_asstr(x));
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_SET_VALUE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = POP();
      z = POP();
      Object_SetValue(y, string_asstr(x), z);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_RETURN_VALUE: {
      x = POP();
      loopflag = 0;
      break;
    }
    case OP_RETURN: {
      x = NULL;
      loopflag = 0;
      break;
    }
    case OP_CALL: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      oparg = NEXT_BYTE();
      y = POP();
      if (oparg == 0) {
        z = NULL;
      } else if (oparg == 1) {
        z = POP();
      } else {
        z = Tuple_New(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          Tuple_Set(z, i, v);
          OB_DECREF(v);
        }
      }
      ks->top = top - base;
      w = Object_Call(y, string_asstr(x), z);
      PUSH(w);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_PRINT: {
      x = POP();
      IoPrintln(x);
      OB_DECREF(x);
      break;
    }
    case OP_TYPEOF: {
      x = POP();
      oparg = NEXT_2BYTES();
      y = Tuple_Get(consts, oparg);
      int bval = typecheck(x, y);
      z = bval ? Bool_True() : Bool_False();
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_TYPECHECK: {
      x = TOP();
      oparg = NEXT_2BYTES();
      y = Tuple_Get(consts, oparg);
      if (!typecheck(x, y))
        panic("typecheck failed");
      OB_DECREF(y);
      break;
    }
    case OP_ADD: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, add);
      call_op_func(z, x, OP_ADD, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_SUB: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, sub);
      call_op_func(z, x, OP_SUB, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_MUL: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, mul);
      call_op_func(z, x, OP_MUL, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_DIV: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, div);
      call_op_func(z, x, OP_DIV, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_MOD: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, mod);
      call_op_func(z, x, OP_MOD, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_POW: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, pow);
      call_op_func(z, x, OP_POW, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEG: {
      x = POP();
      fn = OB_NUM_FUNC(x, neg);
      call_op_func(z, x, OP_NEG, NULL);
      OB_DECREF(x);
      PUSH(z);
      break;
    }
    case OP_GT: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, gt);
      call_op_func(z, x, OP_GT, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_GE: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, ge);
      call_op_func(z, x, OP_GE, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_LT: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, lt);
      call_op_func(z, x, OP_LT, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_LE: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, le);
      call_op_func(z, x, OP_LE, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_EQ: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, eq);
      call_op_func(z, x, OP_EQ, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEQ: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, neq);
      call_op_func(z, x, OP_NEQ, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_AND: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, and);
      call_op_func(z, x, OP_BIT_AND, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_OR: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, or);
      call_op_func(z, x, OP_BIT_OR, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_XOR: {
      x = POP();
      y = POP();
      fn = OB_NUM_FUNC(x, xor);
      call_op_func(z, x, OP_BIT_XOR, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_NOT: {
      x = POP();
      fn = OB_NUM_FUNC(x, not);
      call_op_func(z, x, OP_BIT_NOT, NULL);
      OB_DECREF(x);
      PUSH(z);
      break;
    }
    case OP_AND: {
      x = POP();
      y = POP();
      int r = logic_true(x) && logic_true(y);
      z = r ? Bool_True() : Bool_False();
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_OR: {
      x = POP();
      y = POP();
      int r = logic_true(x) || logic_true(y);
      z = r ? Bool_True() : Bool_False();
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_NOT: {
      x = POP();
      int r = logic_true(x);
      z = r ? Bool_False() : Bool_True();
      PUSH(z);
      OB_DECREF(x);
      break;
    }
    case OP_INPLACE_ADD: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, add);
      call_op_func(z, x, OP_INPLACE_ADD, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_SUB: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, sub);
      call_op_func(z, x, OP_INPLACE_SUB, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_MUL: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, mul);
      call_op_func(z, x, OP_INPLACE_MUL, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_DIV: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, div);
      call_op_func(z, x, OP_INPLACE_DIV, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_POW: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, pow);
      call_op_func(z, x, OP_INPLACE_POW, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_MOD: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, mod);
      call_op_func(z, x, OP_INPLACE_MOD, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_AND: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, and);
      call_op_func(z, x, OP_INPLACE_AND, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_OR: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, or);
      call_op_func(z, x, OP_INPLACE_OR, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_XOR: {
      x = POP();
      y = POP();
      fn = OB_INPLACE_FUNC(x, xor);
      call_op_func(z, x, OP_INPLACE_XOR, y);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_SUBSCR_LOAD: {
      x = POP();
      y = POP();
      fn = OB_MAP_FUNC(x, getitem);
      call_op_func(z, x, OP_SUBSCR_LOAD, y);
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_SUBSCR_STORE: {
      x = POP();
      y = POP();
      z = POP();
      v = Tuple_New(2);
      Tuple_Set(v, 0, y);
      Tuple_Set(v, 1, z);
      fn = OB_MAP_FUNC(x, setitem);
      call_op_func(w, x, OP_SUBSCR_STORE, v);
      PUSH(w);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      OB_DECREF(v);
      break;
    }
    case OP_JMP: {
      oparg = (int16_t)NEXT_2BYTES();
      f->index += oparg;
      break;
    }
    case OP_JMP_TRUE: {
      oparg = (int16_t)NEXT_2BYTES();
      x = POP();
      if (Bool_IsTrue(x))
        f->index += oparg;
      break;
    }
    case OP_JMP_FALSE: {
      oparg = (int16_t)NEXT_2BYTES();
      x = POP();
      if (Bool_IsFalse(x))
        f->index += oparg;
      break;
    }
    case OP_JMP_CMPEQ: {
      break;
    }
    case OP_JMP_CMPNEQ: {
      break;
    }
    case OP_JMP_CMPLT: {
      break;
    }
    case OP_JMP_CMPGT: {
      break;
    }
    case OP_JMP_CMPLE: {
      break;
    }
    case OP_JMP_CMPGE: {
      break;
    }
    case OP_JMP_NIL: {
      oparg = NEXT_2BYTES();
      break;
    }
    case OP_JMP_NOTNIL: {
      oparg = NEXT_2BYTES();
      break;
    }
    case OP_NEW_OBJECT: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        y = NULL;
      } else {
        expect(oparg == 1);
        y = POP();
      }
      desc = descob_getdesc(x);
      expect(desc->paras == NULL);
      if (desc_isbase(desc)) {
        expect(desc = OB_TYPE(y)->desc);
        z = OB_INCREF(y);
      } else {
        expect(desc->kind == TYPE_KLASS);
        expect(y == NULL);
        z = new_object(desc);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEW_TUPLE: {
      oparg = NEXT_2BYTES();
      x = Tuple_New(oparg);
      for (i = 0; i < oparg; ++i) {
        y = POP();
        Tuple_Set(x, i, y);
        OB_DECREF(y);
      }
      PUSH(x);
      break;
    }
    case OP_NEW_ARRAY: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      desc = descob_getdesc(x);
      expect(desc->paras == NULL);
      expect(desc->types != NULL);
      xdesc = vector_get(desc->types, 0);
      y = array_new(xdesc);
      OB_DECREF(x);
      oparg = NEXT_2BYTES();
      for (i = 0; i < oparg; ++i) {
        z = POP();
        array_set(y, i, z);
        OB_DECREF(z);
      }
      PUSH(y);
      break;
    }
    case OP_NEW_MAP: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      desc = descob_getdesc(x);
      expect(desc->paras == NULL);
      expect(desc->types != NULL);
      xdesc = vector_get(desc->types, 0);
      ydesc = vector_get(desc->types, 1);
      v = map_new(xdesc, ydesc);
      OB_DECREF(x);
      oparg = NEXT_BYTE();
      for (i = 0; i < oparg; ++i) {
        x = POP();
        y = POP();
        map_put(v, x, y);
        OB_DECREF(x);
        OB_DECREF(y);
      }
      PUSH(v);
      break;
    }
    case OP_NEW_RANGE: {
      oparg = NEXT_BYTE();
      x = POP();
      y = POP();
      int64_t start = integer_asint(x);
      int64_t end = integer_asint(y);
      if (start > end) {
        z = range_new(x, 0);
      } else {
        z = range_new(x, end - start - oparg + 1);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_ITER: {
      x = POP();
      y = POP();
      if (OB_TYPE(x)->iter != NULL) {
        v = OB_TYPE(x)->iter(x, y);
      } else {
        panic("object is not iteratable.");
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(v);
      break;
    }
    case OP_FOR_ITER: {
      oparg = (int16_t)NEXT_2BYTES();
      x = TOP();
      if (OB_TYPE(x)->iternext != NULL) {
        y = OB_TYPE(x)->iternext(x, NULL);
      } else {
        panic("object is not iteratable.");
      }
      if (y != NULL) {
        PUSH(y);
      } else {
        debug("iterator is ended.");
        x = POP();
        OB_DECREF(x);
        f->index += oparg;
      }
      break;
    }
    case OP_UNPACK_TUPLE: {
      x = POP();
      tuple_for_each(y, x) {
        PUSH(y);
      }
      OB_DECREF(x);
      break;
    }
    default: {
      panic("unknown opcode: %d", op);
      break;
    }
    }
  }

  ks->frame = f->back;
  --ks->depth;
  free_frame(f);
  return x;
}

pthread_key_t kskey;

Object *Koala_EvalCode(Object *self, Object *ob, Object *args)
{
  KoalaState *ks = pthread_getspecific(kskey);
  expect(ks != NULL);
  CodeObject *co = (CodeObject *)self;
  Frame *f = new_frame(ks, self, co->locals);
  prepare_args(f, ob, args);
  return Koala_EvalFrame(f);
}
