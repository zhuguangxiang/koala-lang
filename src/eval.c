/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <math.h>
#include <pthread.h>
#include "eval.h"
#include "codeobject.h"
#include "intobject.h"
#include "floatobject.h"
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
  Object *x, *y, *z, *o, *p, *q;

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
          o = POP();
          Tuple_Set(z, i, o);
          OB_DECREF(o);
        }
      }
      p = Object_Call(y, String_AsStr(x), z);
      PUSH(p);
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
    case OP_ADD: {
      x = POP();
      y = POP();
      if (Integer_Check(x) && Integer_Check(y)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        b = Integer_AsInt(y);
        //overflow?
        r = (int64_t)((uint64_t)a + b);
        z = Integer_New(r);
      } else if (String_Check(x) && String_Check(y)) {
        STRBUF(sbuf);
        strbuf_append(&sbuf, String_AsStr(x));
        strbuf_append(&sbuf, String_AsStr(y));
        String_Set(x, strbuf_tostr(&sbuf));
        strbuf_fini(&sbuf);
        z = OB_INCREF(x);
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
      if (Integer_Check(x) && Integer_Check(y)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        b = Integer_AsInt(y);
        //overflow?
        r = (int64_t)((uint64_t)a - b);
        z = Integer_New(r);
      } else {
        z = Object_Call(x, opcode_map(OP_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
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
    case OP_INPLACE_ADD: {
      x = POP();
      y = POP();
      if (Integer_Check(x)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        if (Integer_Check(y)) {
          b = Integer_AsInt(y);
        } else if (Byte_Check(y)) {
          b = Byte_AsInt(y);
        } else if (Float_Check(y)) {
          b = (int64_t)Float_AsFlt(y);
        } else {
          panic("not implemented");
        }
        //overflow?
        r = (int64_t)((uint64_t)a + b);
        Integer_Set(x, r);
      } else if (Float_Check(x)) {
        double a, r;
        a = Float_AsFlt(x);
        if (Float_Check(y)) {
          double b = Float_AsFlt(y);
          r = a + b;
        } else if (Integer_Check(y)) {
          int64_t b = Integer_AsInt(y);
          r = a + b;
        } else if (Byte_Check(y)) {
          int b = Byte_AsInt(y);
          r = a + b;
        } else {
          panic("not implemented");
        }
        Float_Set(x, r);
      } else if (String_Check(x) && String_Check(y)) {
        STRBUF(sbuf);
        strbuf_append(&sbuf, String_AsStr(x));
        strbuf_append(&sbuf, String_AsStr(y));
        String_Set(x, strbuf_tostr(&sbuf));
        strbuf_fini(&sbuf);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_ADD), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_SUB: {
      x = POP();
      y = POP();
      if (Integer_Check(x)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        if (Integer_Check(y)) {
          b = Integer_AsInt(y);
          r = (int64_t)((uint64_t)a - b);
        } else if (Byte_Check(y)) {
          b = Byte_AsInt(y);
          r = (int64_t)((uint64_t)a - b);
        } else if (Float_Check(y)) {
          double f = Float_AsFlt(y);
          r = (int64_t)((double)a - f);
        } else {
          panic("not implemented");
        }
        Integer_Set(x, r);
      } else if (Float_Check(x)) {
        double a, r;
        a = Float_AsFlt(x);
        if (Float_Check(y)) {
          double b = Float_AsFlt(y);
          r = a - b;
        } else if (Integer_Check(y)) {
          int64_t b = Integer_AsInt(y);
          r = a - b;
        } else if (Byte_Check(y)) {
          int b = Byte_AsInt(y);
          r = a - b;
        } else {
          panic("not implemented");
        }
        Float_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_MUL: {
      x = POP();
      y = POP();
      if (Integer_Check(x)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        if (Integer_Check(y)) {
          b = Integer_AsInt(y);
          r = (int64_t)((uint64_t)a * b);
        } else if (Byte_Check(y)) {
          b = Byte_AsInt(y);
          r = (int64_t)((uint64_t)a * b);
        } else if (Float_Check(y)) {
          double f = Float_AsFlt(y);
          r = (int64_t)((double)a * f);
        } else {
          panic("not implemented");
        }
        Integer_Set(x, r);
      } else if (Float_Check(x)) {
        double a, r;
        a = Float_AsFlt(x);
        if (Float_Check(y)) {
          double b = Float_AsFlt(y);
          r = a * b;
        } else if (Integer_Check(y)) {
          int64_t b = Integer_AsInt(y);
          r = a * b;
        } else if (Byte_Check(y)) {
          int b = Byte_AsInt(y);
          r = a * b;
        } else {
          panic("not implemented");
        }
        Float_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_DIV: {
      x = POP();
      y = POP();
      if (Integer_Check(x)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        if (Integer_Check(y)) {
          b = Integer_AsInt(y);
          r = (int64_t)((uint64_t)a / b);
        } else if (Byte_Check(y)) {
          b = Byte_AsInt(y);
          r = (int64_t)((uint64_t)a / b);
        } else if (Float_Check(y)) {
          double f = Float_AsFlt(y);
          r = (int64_t)((double)a / f);
        } else {
          panic("not implemented");
        }
        Integer_Set(x, r);
      } else if (Float_Check(x)) {
        double a, r;
        a = Float_AsFlt(x);
        if (Float_Check(y)) {
          double b = Float_AsFlt(y);
          r = a / b;
        } else if (Integer_Check(y)) {
          int64_t b = Integer_AsInt(y);
          r = a / b;
        } else if (Byte_Check(y)) {
          int b = Byte_AsInt(y);
          r = a / b;
        } else {
          panic("not implemented");
        }
        Float_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_POW: {
      x = POP();
      y = POP();
      if (Integer_Check(x)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        if (Integer_Check(y)) {
          b = Integer_AsInt(y);
          r = (int64_t)powl(a, b);
        } else if (Byte_Check(y)) {
          b = Byte_AsInt(y);
          r = (int64_t)powl(a, b);
        } else if (Float_Check(y)) {
          double f = Float_AsFlt(y);
          r = (int64_t)powl(a, f);
        } else {
          panic("not implemented");
        }
        Integer_Set(x, r);
      } else if (Float_Check(x)) {
        double a, r;
        a = Float_AsFlt(x);
        if (Float_Check(y)) {
          double b = Float_AsFlt(y);
          r = powl(a, b);
        } else if (Integer_Check(y)) {
          int64_t b = Integer_AsInt(y);
          r = powl(a, b);
        } else if (Byte_Check(y)) {
          int b = Byte_AsInt(y);
          r = powl(a, b);
        } else {
          panic("not implemented");
        }
        Float_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_MOD: {
      x = POP();
      y = POP();
      if (Integer_Check(x)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        if (Integer_Check(y)) {
          b = Integer_AsInt(y);
          r = (int64_t)((uint64_t)a % b);
        } else if (Byte_Check(y)) {
          b = Byte_AsInt(y);
          r = (int64_t)((uint64_t)a % b);
        } else if (Float_Check(y)) {
          double f = Float_AsFlt(y);
          r = (int64_t)((uint64_t)a % (int64_t)f);
        } else {
          panic("not implemented");
        }
        Integer_Set(x, r);
      } else if (Float_Check(x)) {
        double a, r;
        a = Float_AsFlt(x);
        if (Float_Check(y)) {
          double b = Float_AsFlt(y);
          r = (int64_t)a % (int64_t)b;
        } else if (Integer_Check(y)) {
          int64_t b = Integer_AsInt(y);
          r = (int64_t)a % (int64_t)b;
        } else if (Byte_Check(y)) {
          int b = Byte_AsInt(y);
          r = (int64_t)a % (int64_t)b;
        } else {
          panic("not implemented");
        }
        Float_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_AND: {
      x = POP();
      y = POP();
      if (Integer_Check(x) && Integer_Check(y)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        b = Integer_AsInt(y);
        r = (int64_t)((uint64_t)a & (uint64_t)b);
        Integer_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_OR: {
      x = POP();
      y = POP();
      if (Integer_Check(x) && Integer_Check(y)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        b = Integer_AsInt(y);
        r = (int64_t)((uint64_t)a | (uint64_t)b);
        Integer_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_SUB), y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_INPLACE_XOR: {
      x = POP();
      y = POP();
      if (Integer_Check(x) && Integer_Check(y)) {
        int64_t a, b, r;
        a = Integer_AsInt(x);
        b = Integer_AsInt(y);
        r = (int64_t)((uint64_t)a ^ (uint64_t)b);
        Integer_Set(x, r);
      } else {
        Object_Call(x, opcode_map(OP_INPLACE_XOR), y);
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
      o = Tuple_New(2);
      Tuple_Set(o, 0, y);
      Tuple_Set(o, 1, z);
      p = Object_Call(x, opcode_map(OP_SUBSCR_STORE), o);
      PUSH(p);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      OB_DECREF(o);
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
        y = POP();
        Tuple_Set(x, i, y);
        OB_DECREF(y);
      }
      PUSH(x);
      break;
    }
    case OP_NEW_ARRAY: {
      x = Array_New();
      oparg = NEXT_2BYTES();
      for (int i = 0; i < oparg; ++i) {
        y = POP();
        Array_Set(x, i, y);
        OB_DECREF(y);
      }
      PUSH(x);
      break;
    }
    case OP_NEW_MAP: {
      x = Dict_New();
      oparg = NEXT_2BYTES();
      for (int i = 0; i < oparg; ++i) {
        y = POP();
        z = Tuple_Get(y, 0);
        o = Tuple_Get(y, 1);
        Dict_Put(x, z, o);
        OB_DECREF(z);
        OB_DECREF(o);
        OB_DECREF(y);
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
  return x;
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