/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <pthread.h>
#include "eval.h"
#include "codeobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "arrayobject.h"
#include "dictobject.h"
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
  if (ks->depth >= MAX_FRAME_DEPTH) {
    panic("StackOverflow");
  }

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
      Object *o;
      int size = Tuple_Size(args);
      if (f->size != size + 1)
        panic("count of args error");
      for (int i = 0; i < size; i++) {
        o = Tuple_Get(args, i);
        f->locvars[i + 1] = OB_INCREF(o);
        OB_DECREF(o);
      }
    } else {
      if (f->size != 2)
        panic("count of args is not 2");
      f->locvars[1] = OB_INCREF(args);
    }
  }
}

/* global KoalaState list */
static struct list_head kslist;

#define TOP() ({                    \
  if (top >= base + MAX_STACK_SIZE) \
    panic("stack out of range");    \
  *top;                             \
})

#define POP() ({             \
  if (top < base)            \
    panic("stack is empty"); \
  *top--;                    \
})

#define PUSH(v) ({                      \
  if (top >= base + MAX_STACK_SIZE - 1) \
    panic("stack is full");             \
  *++top = v;                           \
})

#define NEXT_OP() ({        \
  if (f->index >= co->size) \
    panic("out of range");  \
  co->codes[++f->index];    \
})

#define NEXT_BYTE() ({      \
  if (f->index >= co->size) \
    panic("out of range");  \
  co->codes[++f->index];    \
})

#define NEXT_2BYTES() ({             \
  if (f->index >= co->size)          \
    panic("out of range");           \
  uint8_t l = co->codes[++f->index]; \
  uint8_t h = co->codes[++f->index]; \
  ((h << 8) + l);                    \
})

#define GETLOCAL(i) ({           \
  if (i >= f->size)              \
    panic("index out of range"); \
  f->locvars[i];                 \
})

#define SETLOCAL(i, v) ({        \
  if (i >= f->size)              \
    panic("index out of range"); \
  f->locvars[i] = v;             \
})

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
  Object *v, *v2, *x;
  Object *retval;

  while (loopflag) {
    if ((f->index + 1) >= co->size) {
      loopflag = 0;
      break;
    }
    op = NEXT_OP();
    switch (op) {
    case OP_POP_TOP: {
      v = POP();
      OB_DECREF(v);
      break;
    }
    case OP_DUP: {
      v = TOP();
      PUSH(OB_INCREF(v));
      break;
    }
    case OP_CONST_BYTE: {
      oparg = NEXT_BYTE();
      v = Integer_New(oparg);
      PUSH(v);
      break;
    }
    case OP_CONST_SHORT: {
      oparg = NEXT_2BYTES();
      v = Integer_New(oparg);
      PUSH(v);
      break;
    }
    case OP_LOAD_CONST: {
      oparg = NEXT_2BYTES();
      v = Tuple_Get(consts, oparg);
      PUSH(v);
      break;
    }
    case OP_LOAD_MODULE: {
      char *path;
      oparg = NEXT_2BYTES();
      v = Tuple_Get(consts, oparg);
      path = String_AsStr(v);
      x = Module_Load(path);
      PUSH(x);
      OB_DECREF(v);
      break;
    }
    case OP_LOAD: {
      oparg = NEXT_BYTE();
      v = GETLOCAL(oparg);
      PUSH(OB_INCREF(v));
      break;
    }
    case OP_LOAD_0: {
      v = GETLOCAL(0);
      PUSH(OB_INCREF(v));
      break;
    }
    case OP_LOAD_1: {
      v = GETLOCAL(1);
      PUSH(OB_INCREF(v));
      break;
    }
    case OP_LOAD_2: {
      v = GETLOCAL(2);
      PUSH(OB_INCREF(v));
      break;
    }
    case OP_LOAD_3: {
      v = GETLOCAL(3);
      PUSH(OB_INCREF(v));
      break;
    }
    case OP_STORE: {
      oparg = NEXT_BYTE();
      v = POP();
      SETLOCAL(oparg, v);
      break;
    }
    case OP_STORE_0: {
      v = POP();
      SETLOCAL(0, v);
      break;
    }
    case OP_STORE_1: {
      v = POP();
      SETLOCAL(1, v);
      break;
    }
    case OP_STORE_2: {
      v = POP();
      SETLOCAL(2, v);
      break;
    }
    case OP_STORE_3: {
      v = POP();
      SETLOCAL(3, v);
      break;
    }
    case OP_GET_OBJECT: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      v = POP();
      retval = Object_Lookup(v, String_AsStr(x));
      PUSH(retval);
      OB_DECREF(x);
      OB_DECREF(v);
      break;
    }
    case OP_GET_FIELD_VALUE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      v = POP();
      retval = Object_GetValue(v, String_AsStr(x));
      PUSH(retval);
      OB_DECREF(x);
      OB_DECREF(v);
      break;
    }
    case OP_SET_FIELD_VALUE: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      v2 = POP();
      v = POP();
      Object_SetValue(v2, String_AsStr(x), v);
      OB_DECREF(x);
      OB_DECREF(v2);
      OB_DECREF(v);
      break;
    }
    case OP_RETURN_VALUE: {
      retval = POP();
      loopflag = 0;
      break;
    }
    case OP_CALL: {
      oparg = NEXT_2BYTES();
      x = Tuple_Get(consts, oparg);
      oparg = NEXT_BYTE();
      v2 = POP();
      if (oparg == 0) {
        v = NULL;
      } else if (oparg == 1) {
        v = POP();
      } else {
        Object *arg;
        v = Tuple_New(oparg);
        for (int i = 0; i < oparg; ++i) {
          arg = POP();
          Tuple_Set(v, i, arg);
          OB_DECREF(arg);
        }
      }
      retval = Object_Call(v2, String_AsStr(x), v);
      PUSH(retval);
      OB_DECREF(x);
      OB_DECREF(v2);
      OB_DECREF(v);
      break;
    }
    case OP_PRINT: {
      v = POP();
      IoPrintln(v);
      OB_DECREF(v);
      break;
    }
    case OP_ADD: {
      v2 = POP();
      v = POP();
      if (Integer_Check(v) && Integer_Check(v2)) {
        int64_t a, b, r;
        a = Integer_AsInt(v);
        b = Integer_AsInt(v2);
        //overflow?
        r = (int64_t)((uint64_t)a + b);
        x = Integer_New(r);
      } else if (String_Check(v) && String_Check(v2)) {

      } else {
        //x = Number_Add(v, w);
      }
      OB_DECREF(v2);
      OB_DECREF(v);
      PUSH(x);
      break;
    }
    case OP_SUB: {
      v2 = POP();
      v = POP();
      if (Integer_Check(v) && Integer_Check(v2)) {
        int64_t a, b, r;
        a = Integer_AsInt(v);
        b = Integer_AsInt(v2);
        //overflow?
        r = (int64_t)((uint64_t)a - b);
        x = Integer_New(r);
      } else {
        //x = Number_Sub(v, w);
      }
      OB_DECREF(v2);
      OB_DECREF(v);
      PUSH(x);
      break;
    }
    case OP_MUL: {
      break;
    }
    case OP_DIV: {
      break;
    }
    case OP_MOD: {
      break;
    }
    case OP_POW: {
      break;
    }
    case OP_NEG: {
      break;
    }
    case OP_GT: {
      break;
    }
    case OP_GE: {
      break;
    }
    case OP_LT: {
      break;
    }
    case OP_LE: {
      break;
    }
    case OP_EQ: {
      break;
    }
    case OP_NEQ: {
      break;
    }
    case OP_BAND: {
      break;
    }
    case OP_BOR: {
      break;
    }
    case OP_BXOR: {
      break;
    }
    case OP_BNOT: {
      break;
    }
    case OP_AND: {
      break;
    }
    case OP_OR: {
      break;
    }
    case OP_NOT: {
      break;
    }
    case OP_SUBSCR_LOAD: {
      v2 = POP();
      v = POP();
      retval = Object_Call(v2, "__getitem__", v);
      PUSH(retval);
      OB_DECREF(v2);
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
    case OP_NEW_TUPLE: {
      oparg = NEXT_2BYTES();
      x = Tuple_New(oparg);
      for (int i = 0; i < oparg; ++i) {
        v = POP();
        Tuple_Set(x, i, v);
        OB_DECREF(v);
      }
      PUSH(x);
      break;
    }
    case OP_NEW_ARRAY: {
      x = Array_New();
      oparg = NEXT_2BYTES();
      for (int i = 0; i < oparg; ++i) {
        v = POP();
        Array_Set(x, i, v);
        OB_DECREF(v);
      }
      PUSH(x);
      break;
    }
    case OP_NEW_MAP: {
      Object *key, *val;
      x = Dict_New();
      oparg = NEXT_2BYTES();
      for (int i = 0; i < oparg; ++i) {
        v = POP();
        key = Tuple_Get(v, 0);
        val = Tuple_Get(v, 1);
        Dict_Put(x, key, val);
        OB_DECREF(key);
        OB_DECREF(val);
        OB_DECREF(v);
      }
      PUSH(x);
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
  return retval;
}

pthread_key_t kskey;

Object *Koala_EvalCode(Object *self, Object *ob, Object *args)
{
  KoalaState *ks = pthread_getspecific(kskey);
  if (ks == NULL)
    panic("ks in pthread is null");
  CodeObject *co = (CodeObject *)self;
  Frame *f = new_frame(ks, self, co->locals);
  prepare_args(f, ob, args);
  Object *res = Koala_EvalFrame(f);
  if (ks->frame)
    panic("ks->frame is not null");
  return res;
}