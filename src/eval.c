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
#include "moduleobject.h"
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
  Object *v, *w, *x;
  Object *retval;

  while (loopflag) {
    if ((f->index + 1) >= co->size) {
      loopflag = 0;
      break;
    }
    op = NEXT_OP();
    switch (op) {
    case POP_TOP: {
      v = POP();
      OB_DECREF(v);
      break;
    }
    case DUP: {
      v = TOP();
      PUSH(OB_INCREF(v));
      break;
    }
    case CONST_BYTE: {
      oparg = NEXT_BYTE();
      v = Integer_New(oparg);
      PUSH(v);
      break;
    }
    case CONST_SHORT: {
      oparg = NEXT_2BYTES();
      v = Integer_New(oparg);
      PUSH(v);
      break;
    }
    case LOAD_CONST: {
      oparg = NEXT_2BYTES();
      v = Tuple_Get(consts, oparg);
      PUSH(v);
      break;
    }
    case LOAD_MODULE: {
      char *path;
      Object *mo;
      oparg = NEXT_2BYTES();
      v = Tuple_Get(consts, oparg);
      path = String_AsStr(v);
      mo = Module_Load(path);
      PUSH(mo);
      OB_DECREF(v);
      break;
    }
    case LOAD: {
      oparg = NEXT_BYTE();
      v = GETLOCAL(oparg);
      PUSH(OB_INCREF(v));
      break;
    }
    case LOAD_0: {
      v = GETLOCAL(0);
      PUSH(OB_INCREF(v));
      break;
    }
    case LOAD_1: {
      v = GETLOCAL(1);
      PUSH(OB_INCREF(v));
      break;
    }
    case LOAD_2: {
      v = GETLOCAL(2);
      PUSH(OB_INCREF(v));
      break;
    }
    case LOAD_3: {
      v = GETLOCAL(3);
      PUSH(OB_INCREF(v));
      break;
    }
    case STORE: {
      oparg = NEXT_BYTE();
      v = POP();
      SETLOCAL(oparg, v);
      break;
    }
    case STORE_0: {
      v = POP();
      SETLOCAL(0, v);
      break;
    }
    case STORE_1: {
      v = POP();
      SETLOCAL(1, v);
      break;
    }
    case STORE_2: {
      v = POP();
      SETLOCAL(2, v);
      break;
    }
    case STORE_3: {
      v = POP();
      SETLOCAL(3, v);
      break;
    }
    case GET_FIELD: {
      Object *name;
      Object *ob;
      oparg = NEXT_2BYTES();
      name = Tuple_Get(consts, oparg);
      ob = POP();
      v = Object_GetValue(ob, String_AsStr(name));
      PUSH(OB_INCREF(v));
      OB_DECREF(name);
      OB_DECREF(ob);
      break;
    }
    case SET_FIELD: {
      Object *name;
      Object *ob;
      oparg = NEXT_2BYTES();
      name = Tuple_Get(consts, oparg);
      ob = POP();
      v = POP();
      Object_SetValue(ob, String_AsStr(name), v);
      OB_DECREF(name);
      OB_DECREF(ob);
      OB_DECREF(v);
      break;
    }
    case CALL: {
      Object *name;
      Object *ob;
      oparg = NEXT_2BYTES();
      name = Tuple_Get(consts, oparg);
      oparg = NEXT_BYTE();
      ob = POP();
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
      retval = Object_Call(ob, String_AsStr(name), v);
      PUSH(OB_INCREF(retval));
      OB_DECREF(ob);
      OB_DECREF(v);
      break;
    }
    case RETURN_VALUE: {
      retval = POP();
      loopflag = 0;
      break;
    }
    case ADD: {
      w = POP();
      v = POP();
      if (Integer_Check(v) && Integer_Check(w)) {
        int64_t a, b, r;
        a = Integer_AsInt(v);
        b = Integer_AsInt(w);
        //overflow?
        r = (int64_t)((uint64_t)a + b);
        x = Integer_New(r);
      } else if (String_Check(v) && String_Check(w)) {

      } else {
        //x = Number_Add(v, w);
      }
      OB_DECREF(w);
      OB_DECREF(v);
      PUSH(x);
      break;
    }
    case SUB: {
      w = POP();
      v = POP();
      if (Integer_Check(v) && Integer_Check(w)) {
        int64_t a, b, r;
        a = Integer_AsInt(v);
        b = Integer_AsInt(w);
        //overflow?
        r = (int64_t)((uint64_t)a - b);
        x = Integer_New(r);
      } else {
        //x = Number_Sub(v, w);
      }
      OB_DECREF(w);
      OB_DECREF(v);
      PUSH(x);
      break;
    }
    case MUL: {
      break;
    }
    case DIV: {
      break;
    }
    case MOD: {
      break;
    }
    case POW: {
      break;
    }
    case NEG: {
      break;
    }
    case GT: {
      break;
    }
    case GE: {
      break;
    }
    case LT: {
      break;
    }
    case LE: {
      break;
    }
    case EQ: {
      break;
    }
    case NEQ: {
      break;
    }
    case BAND: {
      break;
    }
    case BOR: {
      break;
    }
    case BXOR: {
      break;
    }
    case BNOT: {
      break;
    }
    case AND: {
      break;
    }
    case OR: {
      break;
    }
    case NOT: {
      break;
    }
    case JMP: {
      break;
    }
    case JMP_TRUE: {
      break;
    }
    case JMP_FALSE: {
      break;
    }
    case JMP_CMPEQ: {
      break;
    }
    case JMP_CMPNEQ: {
      break;
    }
    case JMP_CMPLT: {
      break;
    }
    case JMP_CMPGT: {
      break;
    }
    case JMP_CMPLE: {
      break;
    }
    case JMP_CMPGE: {
      break;
    }
    case JMP_NIL: {
      break;
    }
    case JMP_NOTNIL: {
      break;
    }
    case NEW: {
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