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
  Object *locvars[0];
} Frame;

static Frame *new_frame(KoalaState *ks, Object *code, int locsize)
{
  bug(ks->depth >= MAX_FRAME_DEPTH, "StackOverflow");

  locsize += 1; /* self object */

  Frame *f = kmalloc(sizeof(Frame) + locsize * sizeof(Object *));
  f->code = OB_INCREF(code);
  f->size = locsize;
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
      bug(f->size != size + 1, "count of args error");
      for (int i = 0; i < size; i++) {
        v = Tuple_Get(args, i);
        f->locvars[i + 1] = OB_INCREF(v);
        OB_DECREF(v);
      }
    } else {
      bug(f->size != 2, "count of args is not 2");
      f->locvars[1] = OB_INCREF(args);
    }
  }
}

/* global KoalaState list */
static struct list_head kslist;

#define TOP() ({ \
  bug(top >= base + MAX_STACK_SIZE, "stack out of range"); \
  *top; \
})

#define POP() ({ \
  bug(top < base, "stack is empty"); \
  *top--; \
})

#define PUSH(v) ({ \
  bug(top >= base + MAX_STACK_SIZE - 1, "stack is full"); \
  *++top = v; \
})

#define NEXT_OP() ({ \
  bug(f->index >= co->size, "code out of range"); \
  co->codes[++f->index]; \
})

#define NEXT_BYTE() ({ \
  bug(f->index >= co->size, "code out of range"); \
  co->codes[++f->index]; \
})

#define NEXT_2BYTES() ({ \
  bug(f->index >= co->size, "code out of range"); \
  uint8_t l = co->codes[++f->index]; \
  uint8_t h = co->codes[++f->index]; \
  ((h << 8) + l); \
})

#define GETLOCAL(i) ({ \
  bug(i >= f->size, "index out of range"); \
  f->locvars[i]; \
})

#define SETLOCAL(i, v) ({ \
  bug(i >= f->size, "index out of range"); \
  f->locvars[i] = v; \
})

static int logic_true(Object *ob)
{
  if (ob == NULL)
    return 0;
  if (Bool_Check(ob)) {
    return Bool_IsTrue(ob) ? 1 : 0;
  } else if (Integer_Check(ob)) {
    return Integer_AsInt(ob) ? 1 : 0;
  } else if (Byte_Check(ob)) {
    return Byte_AsInt(ob) ? 1 : 0;
  } else if (String_Check(ob)) {
    return String_IsEmpty(ob) ? 0 : 1;
  } else if (Char_Check(ob)) {
    return Char_AsChar(ob) ? 1 : 0;
  } else {
    panic("Not Implemented");
    return 0;
  }
}

static int typecheck(Object *ob, Object *type)
{
  if (!Desc_Check(type))
    panic("typecheck: para is not DescType");
  TypeDesc *desc = ((DescObject *)type)->desc;
  if (Integer_Check(ob) && desc_isint(desc))
    return 1;
  if (String_Check(ob) && desc_isstr(desc))
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

static Object *new_object(Object *descob, Object *args)
{
  TypeDesc *desc = ((DescObject *)descob)->desc;
  Object *mo = Module_Load(desc->klass.path);
  bug(mo == NULL, "cannot load module '%s'", MODULE_NAME(mo));

  Object *typeob = Module_Lookup(mo, desc->klass.type);
  bug(typeob == NULL, "cannot find type '%s' in '%s'",
    desc->klass.type, MODULE_NAME(mo));

  TypeObject *type = (TypeObject *)typeob;
  if (type == &integer_type) {
    return OB_INCREF(args);
  } else if (type == &string_type) {
    return OB_INCREF(args);
  } else if (type == &bool_type) {
    return OB_INCREF(args);
  } else if (type == &array_type) {
    desc = vector_get(desc->types, 0);
    return array_new(desc);
  } else if (type == &map_type) {
    Object *kob = Tuple_Get(descob, 0);
    Object *vob = Tuple_Get(descob, 1);
    Object *map = map_new(((DescObject *)kob)->desc, ((DescObject *)vob)->desc);
    OB_DECREF(kob);
    OB_DECREF(vob);
    return map;
  } else if (type == &tuple_type) {
    return Tuple_New(0);
  } else {
    return NULL;
  }
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
      x = Integer_New(oparg);
      PUSH(x);
      break;
    }
    case OP_CONST_SHORT: {
      oparg = NEXT_2BYTES();
      x = Integer_New(oparg);
      PUSH(x);
      break;
    }
    case OP_LOAD_CONST: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      PUSH(x);
      break;
    }
    case OP_LOAD_MODULE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = Module_Load(String_AsStr(x));
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
      z = Object_Lookup(y, String_AsStr(x));
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_GET_VALUE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      y = POP();
      z = Object_GetValue(y, String_AsStr(x));
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
      Object_SetValue(y, String_AsStr(x), z);
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
        for (int i = 0; i < oparg; ++i) {
          v = POP();
          Tuple_Set(z, i, v);
          OB_DECREF(v);
        }
      }
      ks->top = top - base;
      w = Object_Call(y, String_AsStr(x), z);
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
      func_t fn = OB_NUM_FUNC(x, add);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_ADD), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_SUB: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, sub);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_MUL: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, mul);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_MUL), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_DIV: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, div);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_DIV), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_MOD: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, mod);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_MOD), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_POW: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, pow);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_POW), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEG: {
      x = POP();
      func_t fn = OB_NUM_FUNC(x, neg);
      if (fn != NULL) {
        z = fn(x, NULL);
      } else {
        z = Object_Call(x, opcode_map(OP_NEG), NULL);
      }
      OB_DECREF(x);
      PUSH(z);
      break;
    }
    case OP_GT: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, gt);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_GT), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_GE: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, ge);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_GE), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_LT: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, lt);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_LT), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_LE: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, le);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_LE), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_EQ: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, eq);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_EQ), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEQ: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, neq);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_NEQ), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_AND: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, and);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_BIT_AND), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_OR: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, or);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_BIT_OR), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_XOR: {
      x = POP();
      y = POP();
      func_t fn = OB_NUM_FUNC(x, xor);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_BIT_XOR), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_BIT_NOT: {
      x = POP();
      func_t fn = OB_NUM_FUNC(x, not);
      if (fn != NULL) {
        z = fn(x, NULL);
      } else {
        z = Object_Call(x, opcode_map(OP_BIT_NOT), NULL);
      }
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
      func_t fn = OB_INPLACE_FUNC(x, add);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_ADD), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_SUB: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, sub);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_MUL: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, mul);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_MUL), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_DIV: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, div);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_DIV), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_POW: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, pow);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_POW), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_MOD: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, mod);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_MOD), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_AND: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, and);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_AND), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_OR: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, or);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_OR), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_XOR: {
      x = POP();
      y = POP();
      func_t fn = OB_INPLACE_FUNC(x, xor);
      if (fn != NULL) {
        z = fn(x, y);
      } else {
        z = Object_Call(x, opcode_map(OP_INPLACE_XOR), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_SUBSCR_LOAD: {
      x = POP();
      y = POP();
      z = Object_Call(x, opcode_map(OP_SUBSCR_LOAD), y);
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
      w = Object_Call(x, opcode_map(OP_SUBSCR_STORE), v);
      PUSH(w);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      OB_DECREF(v);
      break;
    }
    case OP_JMP: {
      break;
    }
    case OP_JMP_TRUE: {
      break;
    }
    case OP_JMP_FALSE: {
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
      break;
    }
    case OP_JMP_NOTNIL: {
      break;
    }
    case OP_NEW_OBJECT: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        y = NULL;
      } else if (oparg == 1) {
        y = POP();
      } else {
        y = Tuple_New(oparg);
        for (int i = 0; i < oparg; ++i) {
          z = POP();
          Tuple_Set(y, i, z);
          OB_DECREF(z);
        }
      }
      z = new_object(x, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEW_TUPLE: {
      oparg = NEXT_2BYTES();
      x = Tuple_New(oparg);
      for (int i = 0; i < oparg; ++i) {
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
      y = array_new(((DescObject *)x)->desc);
      OB_DECREF(x);
      oparg = NEXT_2BYTES();
      for (int i = 0; i < oparg; ++i) {
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
      y = Tuple_Get(x, 0);
      z = Tuple_Get(x, 1);
      v = map_new(((DescObject *)y)->desc, ((DescObject *)z)->desc);
      OB_DECREF(y);
      OB_DECREF(z);
      OB_DECREF(x);
      oparg = NEXT_BYTE();
      for (int i = 0; i < oparg; ++i) {
        x = POP();
        y = POP();
        map_put(v, x, y);
        OB_DECREF(x);
        OB_DECREF(y);
      }
      PUSH(v);
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
  bug(ks == NULL, "ks in pthread is null");
  CodeObject *co = (CodeObject *)self;
  Frame *f = new_frame(ks, self, co->locals);
  prepare_args(f, ob, args);
  return Koala_EvalFrame(f);
}
