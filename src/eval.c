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
#include "enumobject.h"
#include "closureobject.h"
#include "methodobject.h"
#include "iomodule.h"
#include "opcode.h"
#include "image.h"
#include "log.h"

static char *get_freeval_name(CodeObject *co, int index)
{
  LocVar *var;
  vector_for_each(var, &co->locvec) {
    if (var->index == index) {
      debug("load freeval '%s'", var->name);
      return var->name;
    }
  }
  return NULL;
}

static Vector *getfreevars(CallFrame *f, CodeObject *co)
{
  Vector *freevec = NULL;
  if (vector_size(&co->freevec) > 0) {
    freevec = vector_new();
  }

  void *item;
  int index;
  UpVal *up;
  vector_for_each(item, &co->freevec) {
    index = PTR2INT(item);
    expect(index != -1);
    up = upval_new(get_freeval_name(co, index), f->locvars + index);
    vector_push_back(freevec, up);
  }

  return freevec;
}

static CallFrame *new_frame(KoalaState *ks, CodeObject *co)
{
  expect(ks->depth < MAX_FRAME_DEPTH);
  int locsize = vector_size(&co->locvec);
  debug("code '%s' has %d locvars", co->name, locsize);
  CallFrame *f = kmalloc(sizeof(CallFrame) + locsize * sizeof(Object *));
  f->code = OB_INCREF(co);
  f->freevars = getfreevars(f, co);
  f->size = locsize + 1;
  f->index = -1;
  f->back = ks->frame;
  f->ks = ks;
  ks->frame = f;
  ++ks->depth;
  return f;
}

static void free_frame(CallFrame *f)
{
  UpVal *up;
  vector_for_each(up, f->freevars) {
    --up->refcnt;
    expect(up->refcnt >= 0);
    if (up->refcnt == 0) {
      debug("free upval '%s'", up->name);
      upval_free(up);
    } else {
      debug("set upval '%s' as freeval", up->name);
      up->value = *up->ref;
      OB_INCREF(up->value);
      up->ref = &up->value;
    }
  }
  vector_free(f->freevars);

  for (int i = 0; i < f->size; i++) {
    OB_DECREF(f->locvars[i]);
  }
  OB_DECREF(f->code);

  kfree(f);
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
  Object *old = f->locvars[i]; \
  f->locvars[i] = v; \
  OB_DECREF(old); \
})

#define call_op_func(z, x, op, y) \
do { \
  if (fn != NULL) { \
    z = fn(x, y); \
  } else { \
    z = object_call(x, opcode_map(op), y); \
  } \
} while (0)

static int logic_true(Object *ob)
{
  if (ob == NULL)
    return 0;
  if (bool_check(ob)) {
    return bool_istrue(ob) ? 1 : 0;
  } else if (integer_check(ob)) {
    return integer_asint(ob) ? 1 : 0;
  } else if (byte_check(ob)) {
    return byte_asint(ob) ? 1 : 0;
  } else if (string_check(ob)) {
    return string_isempty(ob) ? 0 : 1;
  } else if (char_check(ob)) {
    return char_asch(ob) ? 1 : 0;
  } else {
    panic("Not Implemented");
    return 0;
  }
}

static int typecheck_inherit(TypeObject *tp, TypeDesc *desc)
{
  TypeObject *base;
  vector_for_each_reverse(base, &tp->lro) {
    if (desc_check(base->desc, desc))
      return 1;
  }
  return 0;
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
  if (bool_check(ob) && desc_isbool(desc))
    return 1;

  TypeObject *tp = OB_TYPE(ob);
  if (desc_check(tp->desc, desc)) {
    #if 0
    if (array_check(ob)) {
      ArrayObject *arr = (ArrayObject *)ob;
      return desc_check(arr->desc, vector_get(desc->types->vec, 0));
    }
    #endif
    return 1;
  } else {
    // check inherit
    return typecheck_inherit(tp, desc);
  }
}

static Object *new_object(CallFrame *f, TypeDesc *desc)
{
  Object *ret;
  Object *ob;
  Object *tob;
  if (desc->klass.path != NULL)
    ob = module_load(desc->klass.path);
  else
    ob = OB_INCREF(f->code->module);
  expect(ob != NULL);
  tob = module_lookup(ob, desc->klass.type);
  expect(tob != NULL);
  OB_DECREF(ob);

  TypeObject *type = (TypeObject *)tob;
  if (type == &array_type) {
    TypeDesc *subdesc = vector_get(desc->klass.typeargs, 0);
    ret = array_new(subdesc);
  } else if (type == &map_type) {
    TypeDesc *kdesc = vector_get(desc->klass.typeargs, 0);
    TypeDesc *vdesc = vector_get(desc->klass.typeargs, 1);
    ret= map_new(kdesc, vdesc);
  } else if (type == &tuple_type) {
    ret = tuple_new(0);
  } else {
    expect(type->alloc != NULL);
    ret = type->alloc(type);
  }
  OB_DECREF(tob);
  return ret;
}

static Vector *getupvals(CallFrame *f, Object *ob)
{
  CodeObject *co = (CodeObject *)ob;
  Vector *upvals = NULL;

  if (vector_size(&co->upvec) > 0) {
    upvals = vector_new();
  }

  UpVal *up;
  void *item;
  int index;
  vector_for_each(item, &co->upvec) {
    index = PTR2INT(item);
    expect(index != -1);
    if (index & 0x8000) {
      debug("upval is passed from anonymous");
      up = closure_getupval(f->other, index & ~0x8000);
    } else {
      up = vector_get(f->freevars, index);
    }
    expect(up != NULL);
    ++up->refcnt;
    vector_push_back(upvals, up);
  }

  return upvals;
}

static Object *do_match_enum_args(Object *match, Object *pattern)
{
  Object *name = tuple_get(pattern, 0);
  int res = enum_check_byname(match, name);
  OB_DECREF(name);
  if (!res) {
    return bool_false();
  }

  int size = tuple_size(pattern);
  Object *ob;
  for (int i = 1; i < size; ++i) {
    ob = tuple_get(pattern, i);
    expect(tuple_check(ob));
    Object *idx = tuple_get(ob, 0);
    Object *val = tuple_get(ob, 1);
    OB_DECREF(ob);
    res = enum_check_value(match, idx, val);
    OB_DECREF(idx);
    OB_DECREF(val);
    if (!res) {
      return bool_false();
    }
  }

  return bool_true();
}

static Object *do_match(Object *match, Object *pattern)
{
  if (array_check(pattern)) {
    ArrayObject *arr = (ArrayObject *)pattern;
    Object *z;
    Object *item;
    vector_for_each(item, &arr->items) {
      z = do_match(match, item);
      if (bool_istrue(z)) {
        return z;
      } else {
        OB_DECREF(z);
      }
    }
    return bool_false();
  } else if (range_check(pattern)) {
    return in_range(pattern, match);
  } else if (bool_check(pattern)) {
    return (match == pattern) ? bool_true() : bool_false();
  } else if (type_isenum(OB_TYPE(pattern))) {
    return enum_match(pattern, match);
  } else {
    Object *z;
    func_t fn = OB_TYPE(pattern)->match;
    call_op_func(z, pattern, OP_MATCH, match);
    return z;
  }
}

static Object *do_dot_index(Object *ob, int index)
{
  if (tuple_check(ob)) {
    panic("tuple index not implemented.");
  } else {
    expect(type_isenum(OB_TYPE(ob)));
    return enum_get_value(ob, index);
  }
}

static inline TypeObject *get_parent_type(TypeObject *cls, TypeObject *type)
{
  expect(type_isclass(cls));
  TypeObject *parent = type_parent(cls, type);
  expect(parent != NULL && parent != type);
  return parent;
}

Object *Koala_EvalFrame(CallFrame *f)
{
  KoalaState *ks = f->ks;
  Object **base = ks->stack;
  Object **top = base + ks->top;
  CodeObject *co = f->code;
  Object *consts = co->consts;
  uint8_t op;
  int oparg;
  Object *x, *y, *z, *v, *w;
  TypeDesc *desc, *xdesc, *ydesc;
  func_t fn;
  int i;

  while (1) {
    if ((f->index + 1) >= co->size) {
      x = NULL;
      goto exit_loop;
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
    case OP_SWAP: {
      x = POP();
      y = POP();
      PUSH(x);
      PUSH(y);
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
    case OP_CONST_NULL: {
      x = NULL;
      PUSH(x);
      break;
    }
    case OP_LOAD_CONST: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      PUSH(x);
      break;
    }
    case OP_LOAD_MODULE: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = module_load(string_asstr(x));
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
    case OP_GET_METHOD: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = POP();
      z = object_getmethod(y, string_asstr(x));
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_GET_VALUE: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = POP();
      z = object_getvalue(y, string_asstr(x), co->type);
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_SET_VALUE: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = POP();
      z = POP();
      object_setvalue(y, string_asstr(x), z, co->type);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_GET_SUPER_VALUE: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = POP();
      z = object_getvalue(y, string_asstr(x),
                          get_parent_type(OB_TYPE(y), co->type));
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_SET_SUPER_VALUE: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = POP();
      z = POP();
      object_setvalue(y, string_asstr(x), z,
                      get_parent_type(OB_TYPE(y), co->type));
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_RETURN_VALUE: {
      x = POP();
      goto exit_loop;
    }
    case OP_RETURN: {
      x = NULL;
      goto exit_loop;
    }
    case OP_SUPER_CALL: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      oparg = NEXT_BYTE();
      y = POP();
      if (oparg == 0) {
        z = NULL;
      } else if (oparg == 1) {
        z = POP();
      } else {
        z = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(z, i, v);
          OB_DECREF(v);
        }
      }
      ks->top = top - base;
      w = object_super_call(y, string_asstr(x), z,
                            get_parent_type(OB_TYPE(y), co->type));
      PUSH(w);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_CALL:
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      oparg = NEXT_BYTE();
      y = POP();
      if (oparg == 0) {
        z = NULL;
      } else if (oparg == 1) {
        z = POP();
      } else {
        z = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(z, i, v);
          OB_DECREF(v);
        }
      }
      ks->top = top - base;
      w = object_call(y, string_asstr(x), z);
      PUSH(w);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    case OP_EVAL:
      x = POP();
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        y = NULL;
      } else if (oparg == 1) {
        y = POP();
      } else {
        y = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(y, i, v);
          OB_DECREF(v);
        }
      }
      ks->top = top - base;
      if (method_check(x)) {
        debug("call method '%s' argc %d", ((MethodObject *)x)->name, oparg);
        z = method_call(x, NULL, y);
      } else {
        debug("call closure '%s' argc %d", ((ClosureObject *)x)->name, oparg);
        expect(closure_check(x));
        z = koala_evalcode(closure_getcode(x), x, y, x);
      }
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    case OP_PRINT: {
      x = POP();
      IoPrintln(x);
      OB_DECREF(x);
      break;
    }
    case OP_TYPEOF: {
      x = POP();
      oparg = NEXT_2BYTES();
      y = tuple_get(consts, oparg);
      int bval = typecheck(x, y);
      z = bval ? bool_true() : bool_false();
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_TYPECHECK: {
      x = TOP();
      oparg = NEXT_2BYTES();
      y = tuple_get(consts, oparg);
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
      if (x == NULL) {
        z = (y == NULL ? bool_true() : bool_false());
      } else if (y == NULL) {
        z = bool_false();
      } else {
        fn = OB_NUM_FUNC(x, eq);
        call_op_func(z, x, OP_EQ, y);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEQ: {
      x = POP();
      y = POP();
      if (x == NULL) {
        z = (y != NULL ? bool_true() : bool_false());
      } else if (y == NULL) {
        z = bool_true();
      } else {
        fn = OB_NUM_FUNC(x, neq);
        call_op_func(z, x, OP_NEQ, y);
      }
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
      z = r ? bool_true() : bool_false();
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_OR: {
      x = POP();
      y = POP();
      int r = logic_true(x) || logic_true(y);
      z = r ? bool_true() : bool_false();
      PUSH(z);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_NOT: {
      x = POP();
      int r = logic_true(x);
      z = r ? bool_false() : bool_true();
      PUSH(z);
      OB_DECREF(x);
      break;
    }
    case OP_INPLACE_ADD: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, add);
      call_op_func(z, x, OP_ADD, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_SUB: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, sub);
      call_op_func(z, x, OP_SUB, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_MUL: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, mul);
      call_op_func(z, x, OP_MUL, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_DIV: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, div);
      call_op_func(z, x, OP_DIV, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_MOD: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, mod);
      call_op_func(z, x, OP_MOD, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_POW: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, pow);
      call_op_func(z, x, OP_POW, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_AND: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, and);
      call_op_func(z, x, OP_BIT_AND, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_OR: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, or);
      call_op_func(z, x, OP_BIT_OR, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_INPLACE_XOR: {
      y = POP();
      x = POP();
      fn = OB_NUM_FUNC(x, xor);
      call_op_func(z, x, OP_BIT_XOR, y);
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_SUBSCR_LOAD: {
      y = POP();
      x = POP();
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
      v = tuple_new(2);
      tuple_set(v, 0, y);
      tuple_set(v, 1, z);
      fn = OB_MAP_FUNC(x, setitem);
      call_op_func(w, x, OP_SUBSCR_STORE, v);
      expect(w == NULL);
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
      if (bool_istrue(x))
        f->index += oparg;
      OB_DECREF(x);
      break;
    }
    case OP_JMP_FALSE: {
      oparg = (int16_t)NEXT_2BYTES();
      x = POP();
      if (bool_check(x)) {
        if (bool_isfalse(x)) f->index += oparg;
      } else {
        // null or zero?
        if (x == NULL) f->index += oparg;
      }
      OB_DECREF(x);
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
      oparg = (int16_t)NEXT_2BYTES();
      break;
    }
    case OP_JMP_NOTNIL: {
      oparg = (int16_t)NEXT_2BYTES();
      break;
    }
    case OP_NEW_TUPLE: {
      oparg = NEXT_2BYTES();
      x = tuple_new(oparg);
      for (i = 0; i < oparg; ++i) {
        y = POP();
        tuple_set(x, i, y);
        OB_DECREF(y);
      }
      PUSH(x);
      break;
    }
    case OP_NEW_ARRAY: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      desc = descob_getdesc(x);
      //expect(desc->paras == NULL);
      //expect(desc->types != NULL);
      xdesc = vector_get(desc->klass.typeargs, 0);
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
      x = tuple_get(consts, oparg);
      desc = descob_getdesc(x);
      //expect(desc->paras == NULL);
      //expect(desc->types != NULL);
      xdesc = vector_get(desc->klass.typeargs, 0);
      ydesc = vector_get(desc->klass.typeargs, 1);
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
    case OP_NEW_CLOSURE: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      y = closure_new(x, getupvals(f, x));
      OB_DECREF(x);
      PUSH(y);
      break;
    }
    case OP_NEW: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        y = NULL;
      } else {
        expect(oparg == 1);
        y = POP();
      }
      desc = descob_getdesc(x);
      //expect(desc->paras == NULL);
      if (desc_isbase(desc)) {
        expect(desc = OB_TYPE(y)->desc);
        z = OB_INCREF(y);
      } else {
        expect(desc->kind == TYPE_KLASS);
        expect(y == NULL);
        z = new_object(f, desc);
      }
      OB_DECREF(x);
      OB_DECREF(y);
      PUSH(z);
      break;
    }
    case OP_NEW_ITER: {
      x = POP();
      if (OB_TYPE(x)->iter != NULL) {
        v = OB_TYPE(x)->iter(x, NULL);
      } else {
        panic("object is not iteratable.");
      }
      OB_DECREF(x);
      PUSH(v);
      break;
    }
    case OP_FOR_ITER: {
      oparg = (int16_t)NEXT_2BYTES();
      x = POP();
      y = TOP();
      if (OB_TYPE(y)->iternext != NULL) {
        z = OB_TYPE(y)->iternext(y, x);
      } else {
        panic("object is not iteratable.");
      }
      if (z != NULL) {
        PUSH(z);
      } else {
        debug("iterator is ended.");
        f->index += oparg;
      }
      OB_DECREF(x);
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
    case OP_UPVAL_LOAD: {
      oparg = NEXT_BYTE();
      x = POP();
      y = upval_load(x, oparg);
      OB_DECREF(x);
      PUSH(y);
      break;
    }
    case OP_UPVAL_STORE: {
      oparg = NEXT_BYTE();
      x = POP();
      y = POP();
      z = upval_store(x, oparg, y);
      expect(z == NULL);
      OB_DECREF(x);
      OB_DECREF(y);
      break;
    }
    case OP_LOAD_GLOBAL: {
      x = f->code->module;
      PUSH(OB_INCREF(x));
      break;
    }
    case OP_INIT_CALL: {
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        z = NULL;
      } else if (oparg == 1) {
        z = POP();
      } else {
        z = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(z, i, v);
          OB_DECREF(v);
        }
      }
      y = POP();
      ks->top = top - base;
      w = object_call(y, "__init__", z);
      expect(w == NULL);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_SUPER_INIT_CALL: {
      y = POP();
      oparg = NEXT_BYTE();
      if (oparg == 0) {
        z = NULL;
      } else if (oparg == 1) {
        z = POP();
      } else {
        z = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(z, i, v);
          OB_DECREF(v);
        }
      }
      ks->top = top - base;
      w = object_super_call(y, "__init__", z,
                            get_parent_type(OB_TYPE(y), co->type));
      expect(w == NULL);
      PUSH(w);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_NEW_EVAL: {
      oparg = NEXT_2BYTES();
      x = tuple_get(consts, oparg);
      oparg = NEXT_BYTE();
      y = POP();
      if (oparg == 0) {
        z = NULL;
      } else {
        z = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(z, i, v);
          OB_DECREF(v);
        }
      }
      w = enum_new(y, string_asstr(x), z);
      PUSH(w);
      OB_DECREF(x);
      OB_DECREF(y);
      OB_DECREF(z);
      break;
    }
    case OP_MATCH: {
      oparg = NEXT_BYTE();
      if (oparg == 1) {
        y = POP();
      } else {
        expect(oparg >= 2);
        y = tuple_new(oparg);
        for (i = 0; i < oparg; ++i) {
          v = POP();
          tuple_set(y, i, v);
          OB_DECREF(v);
        }
      }
      //x = TOP();
      x = POP();
      z = do_match(x, y);
      OB_DECREF(x);
      OB_DECREF(y);
      /*
      if (bool_isfalse(z)) {
        x = POP();
        OB_DECREF(x);
      }
      */
      PUSH(z);
      break;
    }
    case OP_DOT_INDEX: {
      oparg = NEXT_BYTE();
      x = POP();
      y = do_dot_index(x, oparg);
      PUSH(y);
      OB_DECREF(x);
      break;
    }
    default: {
      panic("unknown opcode: %d", op);
      break;
    }
    }
  }

exit_loop:
  ks->top = top - base;
  ks->frame = f->back;
  --ks->depth;
  free_frame(f);
  return x;
}

pthread_key_t kskey;

Object *koala_evalcode(Object *self, Object *ob, Object *args, Object *other)
{
  KoalaState *ks = pthread_getspecific(kskey);
  expect(ks != NULL);
  CallFrame *f = new_frame(ks, (CodeObject *)self);
  f->other = other;
  f->locvars[0] = OB_INCREF(ob);
  if (args != NULL) {
    if (tuple_check(args)) {
      Object *v;
      int size = tuple_size(args);
      for (int i = 0; i < size; i++) {
        v = tuple_get(args, i);
        f->locvars[i + 1] = OB_INCREF(v);
        OB_DECREF(v);
      }
    } else {
      f->locvars[1] = OB_INCREF(args);
    }
  }
  return Koala_EvalFrame(f);
}
