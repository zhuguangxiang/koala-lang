/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <math.h>
#include "floatobject.h"
#include "intobject.h"
#include "stringobject.h"

static void float_free(Object *ob)
{
  if (!Float_Check(ob)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(ob));
    return;
  }
  FloatObject *f = (FloatObject *)ob;
  debug("[Freed] float %lf", f->value);
  kfree(ob);
}

static Object *float_str(Object *self, Object *ob)
{
  if (!Float_Check(self)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(self));
    return NULL;
  }

  FloatObject *f = (FloatObject *)self;
  char buf[256];
  sprintf(buf, "%lf", f->value);
  return String_New(buf);
}

static double flt_add(Object *x, Object *y)
{
  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = (a + b);
    if ((a > 0 && b > 0 && r < 0) ||
        (a < 0 && b < 0 && r > 0))
      panic("overflow:%lf + %ld = %lf", a, b, r);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = (a + b);
    if ((a > 0 && b > 0 && r < 0) ||
        (a < 0 && b < 0 && r > 0))
      panic("overflow:%lf + %d = %lf", a, b, r);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = (a + b);
    if ((a > 0 && b > 0 && r < 0) ||
        (a < 0 && b < 0 && r > 0))
      panic("overflow:%lf + %lf = %lf", a, b, r);
  } else {
    panic("Not Implemented");
  }

  return r;
}

static double flt_sub(Object *x, Object *y)
{
  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %ld = %lf", a, b, r);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %d = %lf", a, b, r);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %lf = %lf", a, b, r);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static double flt_mul(Object *x, Object *y)
{
  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = a * b;
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = a * b;
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = a * b;
  } else {
    panic("Not Implemented");
  }
  return r;
}

static double flt_div(Object *x, Object *y)
{
  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = a / b;
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = a / b;
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = a / b;
  } else {
    panic("Not Implemented");
  }
  return r;
}

static double flt_mod(Object *x, Object *y)
{
  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = fmod(a, b);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = fmod(a, b);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = fmod(a, b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t flt_pow(Object *x, Object *y)
{
  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = pow(a, b);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = pow(a, b);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = pow(a, b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static Object *flt_num_add(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__add__\" must be two operands");
    return NULL;
  }

  return Float_New(flt_add(x, y));
}

static Object *flt_num_sub(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__sub__\" must be two operands");
    return NULL;
  }

  return Float_New(flt_sub(x, y));
}

static Object *flt_num_mul(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__mul__\" must be two operands");
    return NULL;
  }

  return Float_New(flt_mul(x, y));
}

static Object *flt_num_div(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__div__\" must be two operands");
    return NULL;
  }

  return Float_New(flt_div(x, y));
}

static Object *flt_num_mod(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__mod__\" must be two operands");
    return NULL;
  }

  return Float_New(flt_mod(x, y));
}

static Object *flt_num_pow(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__pow__\" must be two operands");
    return NULL;
  }

  return Float_New(flt_pow(x, y));
}

static Object *flt_num_neg(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y != NULL) {
    error("'-' must be only one operand");
    return NULL;
  }

  Object *z;
  double a = Float_AsFlt(x);
  z = Float_New((double)0 - a);
  return z;
}

static double flt_num_cmp(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    //FIXME: retval?
    return 0;
  }

  double a = Float_AsFlt(x);
  double r;
  if (Integer_Check(y)) {
    int64_t b = Integer_AsInt(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %ld = %lf", a, b, r);
  } else if (Byte_Check(y)) {
    int b = Byte_AsInt(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %d = %lf", a, b, r);
  } else if (Float_Check(y)) {
    double b = Float_AsFlt(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %lf = %lf", a, b, r);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static Object *flt_num_gt(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r > 0) ? Bool_True() : Bool_False();
}

static Object *flt_num_ge(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r >= 0) ? Bool_True() : Bool_False();
}

static Object *flt_num_lt(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r < 0) ? Bool_True() : Bool_False();
}

static Object *flt_num_le(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r <= 0) ? Bool_True() : Bool_False();
}

static Object *flt_num_eq(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r == 0) ? Bool_True() : Bool_False();
}

static Object *flt_num_neq(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r != 0) ? Bool_True() : Bool_False();
}

static NumberMethods float_numbers = {
  .add = flt_num_add,
  .sub = flt_num_sub,
  .mul = flt_num_mul,
  .div = flt_num_div,
  .mod = flt_num_mod,
  .pow = flt_num_pow,
  .neg = flt_num_neg,

  .gt  = flt_num_gt,
  .ge  = flt_num_ge,
  .lt  = flt_num_lt,
  .le  = flt_num_le,
  .eq  = flt_num_eq,
  .neq = flt_num_neq,
};

static Object *flt_num_inadd(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'+=' must be two operands");
    return NULL;
  }

  FloatObject *fo = (FloatObject *)x;
  fo->value = flt_add(x, y);
  return NULL;
}

static Object *flt_num_insub(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'-=' must be two operands");
    return NULL;
  }

  FloatObject *fo = (FloatObject *)x;
  fo->value = flt_sub(x, y);
  return NULL;
}

static Object *flt_num_inmul(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'*=' must be two operands");
    return NULL;
  }

  FloatObject *fo = (FloatObject *)x;
  fo->value = flt_mul(x, y);
  return NULL;
}

static Object *flt_num_indiv(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'/=' must be two operands");
    return NULL;
  }

  FloatObject *fo = (FloatObject *)x;
  fo->value = flt_div(x, y);
  return NULL;
}

static Object *flt_num_inmod(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'%%=' must be two operands");
    return NULL;
  }

  FloatObject *fo = (FloatObject *)x;
  fo->value = flt_mod(x, y);
  return NULL;
}

static Object *flt_num_inpow(Object *x, Object *y)
{
  if (!Float_Check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("'**=' must be two operands");
    return NULL;
  }

  FloatObject *fo = (FloatObject *)x;
  fo->value = flt_pow(x, y);
  return NULL;
}

static InplaceMethods float_inplaces = {
  .add = flt_num_inadd,
  .sub = flt_num_insub,
  .mul = flt_num_inmul,
  .div = flt_num_indiv,
  .mod = flt_num_inmod,
  .pow = flt_num_inpow,
};

TypeObject Float_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Float",
  .free    = float_free,
  .str     = float_str,
  .number  = &float_numbers,
  .inplace = &float_inplaces,
};

void init_float_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Float");
  Float_Type.desc = desc;
  if (type_ready(&Float_Type) < 0)
    panic("Cannot initalize 'Float' type.");
}

Object *Float_New(double val)
{
  FloatObject *f = kmalloc(sizeof(FloatObject));
  init_object_head(f, &Float_Type);
  f->value = val;
  return (Object *)f;
}
