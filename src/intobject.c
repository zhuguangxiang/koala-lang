/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <math.h>
#include "intobject.h"
#include "stringobject.h"
#include "floatobject.h"
#include "fmtmodule.h"

static void integer_free(Object *ob)
{
  if (!Integer_Check(ob)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(ob));
    return;
  }
  debug("[Freed] Integer %ld", Integer_AsInt(ob));
  kfree(ob);
}

static Object *integer_str(Object *self, Object *ob)
{
  if (!Integer_Check(self)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(self));
    return NULL;
  }

  IntegerObject *i = (IntegerObject *)self;
  char buf[256];
  sprintf(buf, "%ld", i->value);
  return String_New(buf);
}

static Object *integer_fmt(Object *self, Object *ob)
{
  if (!Integer_Check(self)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(self));
    return NULL;
  }

  Fmtter_WriteInteger(ob, self);
  return NULL;
}

static int64_t int_add(Object *x, Object *y)
{
  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)((uint64_t)a + b);
    if ((r ^ a) < 0 && (r ^ b) < 0)
      panic("overflow:%ld + %ld = %ld", a, b, r);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)((uint64_t)a + b);
    if ((r ^ a) < 0 && (r ^ b) < 0)
      panic("overflow:%ld + %d = %ld", a, b, r);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)((double)a + b);
    if ((a > 0 && b > 0 && r < 0) ||
        (a < 0 && b < 0 && r > 0))
      panic("overflow:%ld + %lf = %ld", a, b, r);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t int_sub(Object *x, Object *y)
{
  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)((uint64_t)a - b);
    if ((r ^ a) < 0 && (r ^ ~b) < 0)
      panic("overflow:%ld + %ld = %ld", a, b, r);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)((uint64_t)a - b);
    if ((r ^ a) < 0 && (r ^ ~b) < 0)
      panic("overflow:%ld + %d = %ld", a, b, r);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)((double)a - b);
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%ld + %lf = %ld", a, b, r);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t int_mul(Object *x, Object *y)
{
  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)(a * b);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)(a * b);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)(a * b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t int_div(Object *x, Object *y)
{
  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)(a / b);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)(a / b);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)(a / b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t int_mod(Object *x, Object *y)
{
  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)fmod(a, b);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)fmod(a, b);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)fmod(a, b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t int_pow(Object *x, Object *y)
{
  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)pow(a, b);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)pow(a, b);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)pow(a, b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static Object *int_num_add(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__add__\" must be two operands");
    return NULL;
  }

  return Integer_New(int_add(x, y));
}

static Object *int_num_sub(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__sub__\" must be two operands");
    return NULL;
  }

  return Integer_New(int_sub(x, y));
}

static Object *int_num_mul(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__mul__\" must be two operands");
    return NULL;
  }

  return Integer_New(int_mul(x, y));
}

static Object *int_num_div(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__div__\" must be two operands");
    return NULL;
  }

  return Integer_New(int_div(x, y));
}

static Object *int_num_mod(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__mod__\" must be two operands");
    return NULL;
  }

  return Integer_New(int_mod(x, y));
}

static Object *int_num_pow(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__pow__\" must be two operands");
    return NULL;
  }

  return Integer_New(int_pow(x, y));
}

static Object *int_num_neg(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y != NULL) {
    error("'-' must be only one operand");
    return NULL;
  }

  Object *z;
  int64_t a = Integer_AsInt(x);
  z = Integer_New((int64_t)((uint64_t)0 - a));
  return z;
}

static int64_t int_num_cmp(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    //FIXME: retval?
    return 0;
  }

  int64_t a = Integer_AsInt(x);
  int64_t r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (int64_t)((uint64_t)a - b);
    if ((r ^ a) < 0 && (r ^ ~b) < 0)
      panic("overflow:%ld + %ld = %ld", a, b, r);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (int64_t)((uint64_t)a - b);
    if ((r ^ a) < 0 && (r ^ ~b) < 0)
      panic("overflow:%ld + %d = %ld", a, b, r);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (int64_t)((double)a - b);
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%ld + %lf = %ld", a, b, r);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static Object *int_num_gt(Object *x, Object *y)
{
  int64_t r = int_num_cmp(x, y);
  return (r > 0) ? Bool_True() : Bool_False();
}

static Object *int_num_ge(Object *x, Object *y)
{
  int64_t r = int_num_cmp(x, y);
  return (r >= 0) ? Bool_True() : Bool_False();
}

static Object *int_num_lt(Object *x, Object *y)
{
  int64_t r = int_num_cmp(x, y);
  return (r < 0) ? Bool_True() : Bool_False();
}

static Object *int_num_le(Object *x, Object *y)
{
  int64_t r = int_num_cmp(x, y);
  return (r <= 0) ? Bool_True() : Bool_False();
}

static Object *int_num_eq(Object *x, Object *y)
{
  int64_t r = int_num_cmp(x, y);
  return (r == 0) ? Bool_True() : Bool_False();
}

static Object *int_num_neq(Object *x, Object *y)
{
  int64_t r = int_num_cmp(x, y);
  return (r != 0) ? Bool_True() : Bool_False();
}

static Object *int_num_and(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!Integer_Check(y)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(y));
    return NULL;
  }

  Object *z;
  int64_t a = Integer_AsInt(x);
  int64_t b = Integer_AsInt(y);
  z = Integer_New(a & b);
  return z;
}

static Object *int_num_or(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!Integer_Check(y)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(y));
    return NULL;
  }

  Object *z;
  int64_t a = Integer_AsInt(x);
  int64_t b = Integer_AsInt(y);
  z = Integer_New(a | b);
  return z;
}

static Object *int_num_xor(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!Integer_Check(y)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(y));
    return NULL;
  }

  Object *z;
  int64_t a = Integer_AsInt(x);
  int64_t b = Integer_AsInt(y);
  z = Integer_New(a ^ b);
  return z;
}

static Object *int_num_not(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y != NULL) {
    error("'-' must be only one operand");
    return NULL;
  }

  Object *z;
  int64_t a = Integer_AsInt(x);
  z = Integer_New(~a);
  return z;
}

static NumberMethods int_numbers = {
  .add = int_num_add,
  .sub = int_num_sub,
  .mul = int_num_mul,
  .div = int_num_div,
  .mod = int_num_mod,
  .pow = int_num_pow,
  .neg = int_num_neg,

  .gt  = int_num_gt,
  .ge  = int_num_ge,
  .lt  = int_num_lt,
  .le  = int_num_le,
  .eq  = int_num_eq,
  .neq = int_num_neq,

  .and = int_num_and,
  .or  = int_num_or,
  .xor = int_num_xor,
  .not = int_num_not,
};

static Object *int_num_inadd(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'+=' must be two operands");
    return NULL;
  }

  IntegerObject *iob = (IntegerObject *)x;
  iob->value = int_add(x, y);
  return NULL;
}

static Object *int_num_insub(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'-=' must be two operands");
    return NULL;
  }

  IntegerObject *iob = (IntegerObject *)x;
  iob->value = int_sub(x, y);
  return NULL;
}

static Object *int_num_inmul(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'*=' must be two operands");
    return NULL;
  }

  IntegerObject *iob = (IntegerObject *)x;
  iob->value = int_mul(x, y);
  return NULL;
}

static Object *int_num_indiv(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'/=' must be two operands");
    return NULL;
  }

  IntegerObject *iob = (IntegerObject *)x;
  iob->value = int_div(x, y);
  return NULL;
}

static Object *int_num_inmod(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'%%=' must be two operands");
    return NULL;
  }

  IntegerObject *iob = (IntegerObject *)x;
  iob->value = int_mod(x, y);
  return NULL;
}

static Object *int_num_inpow(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'**=' must be two operands");
    return NULL;
  }

  IntegerObject *iob = (IntegerObject *)x;
  iob->value = int_pow(x, y);
  return NULL;
}

static Object *int_num_inand(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!Integer_Check(y)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(y));
    return NULL;
  }

  int64_t a = Integer_AsInt(x);
  int64_t b = Integer_AsInt(y);
  IntegerObject *iob = (IntegerObject *)x;
  iob->value = (a & b);
}

static Object *int_num_inor(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!Integer_Check(y)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(y));
    return NULL;
  }

  int64_t a = Integer_AsInt(x);
  int64_t b = Integer_AsInt(y);
  IntegerObject *iob = (IntegerObject *)x;
  iob->value = (a | b);
}

static Object *int_num_inxor(Object *x, Object *y)
{
  if (!Integer_Check(x)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!Integer_Check(y)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(y));
    return NULL;
  }

  int64_t a = Integer_AsInt(x);
  int64_t b = Integer_AsInt(y);
  IntegerObject *iob = (IntegerObject *)x;
  iob->value = (a ^ b);
}

static InplaceMethods int_inplaces = {
  .add = int_num_inadd,
  .sub = int_num_insub,
  .mul = int_num_inmul,
  .div = int_num_indiv,
  .mod = int_num_inmod,
  .pow = int_num_inpow,

  .and = int_num_inand,
  .or  = int_num_inor,
  .xor = int_num_inxor,
};

static MethodDef int_methods[]= {
  {"__fmt__", "Llang.Formatter;", NULL, integer_fmt},
  {NULL}
};

TypeObject Integer_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Integer",
  .free    = integer_free,
  .str     = integer_str,
  .number  = &int_numbers,
  .inplace = &int_inplaces,
  .methods = int_methods,
};

Object *Integer_New(int64_t val)
{
  IntegerObject *integer = kmalloc(sizeof(*integer));
  Init_Object_Head(integer, &Integer_Type);
  integer->value = val;
  return (Object *)integer;
}

static void byte_free(Object *ob)
{
  if (!Byte_Check(ob)) {
    error("object of '%.64s' is not a Byte", OB_TYPE_NAME(ob));
    return;
  }
  ByteObject *b = (ByteObject *)ob;
  debug("[Freed] Byte %d", b->value);
  kfree(ob);
}

static Object *byte_str(Object *self, Object *ob)
{
  if (!Byte_Check(self)) {
    error("object of '%.64s' is not a Byte", OB_TYPE_NAME(self));
    return NULL;
  }

  ByteObject *b = (ByteObject *)ob;
  char buf[8];
  sprintf(buf, "%d", b->value);
  return String_New(buf);
}

TypeObject Byte_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Byte",
  .free = byte_free,
  .str  = byte_str,
};

Object *Byte_New(int val)
{
  ByteObject *b = kmalloc(sizeof(ByteObject));
  Init_Object_Head(b, &Byte_Type);
  b->value = val;
  return (Object *)b;
}

static Object *bool_str(Object *self, Object *ob)
{
  if (!Bool_Check(self)) {
    error("object of '%.64s' is not a Bool", OB_TYPE_NAME(self));
    return NULL;
  }

  BoolObject *b = (BoolObject *)self;
  char buf[8];
  sprintf(buf, "%s", b->value ? "true" : "false");
  return String_New(buf);
}

TypeObject Bool_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Bool",
  .str  = bool_str,
};

BoolObject OB_True = {
  OBJECT_HEAD_INIT(&Bool_Type)
  .value = 1,
};

BoolObject OB_False = {
  OBJECT_HEAD_INIT(&Bool_Type)
  .value = 0,
};
