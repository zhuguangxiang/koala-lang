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

#include <math.h>
#include "numberobject.h"
#include "floatobject.h"
#include "intobject.h"
#include "stringobject.h"

static void float_free(Object *ob)
{
  if (!float_check(ob)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(ob));
    return;
  }
#if !defined(NLog)
  FloatObject *f = (FloatObject *)ob;
  debug("[Freed] float %lf", f->value);
#endif
  kfree(ob);
}

static Object *float_str(Object *self, Object *ob)
{
  if (!float_check(self)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(self));
    return NULL;
  }

  FloatObject *f = (FloatObject *)self;
  char buf[256];
  sprintf(buf, "%lf", f->value);
  return string_new(buf);
}

static double flt_add(Object *x, Object *y)
{
  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = (a + b);
    if ((a > 0 && b > 0 && r < 0) ||
        (a < 0 && b < 0 && r > 0))
      panic("overflow:%lf + %"PRId64" = %lf", a, b, r);
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = (a + b);
    if ((a > 0 && b > 0 && r < 0) ||
        (a < 0 && b < 0 && r > 0))
      panic("overflow:%lf + %d = %lf", a, b, r);
  } else if (float_check(y)) {
    double b = float_asflt(y);
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
  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %"PRId64" = %lf", a, b, r);
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %d = %lf", a, b, r);
  } else if (float_check(y)) {
    double b = float_asflt(y);
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
  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = a * b;
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = a * b;
  } else if (float_check(y)) {
    double b = float_asflt(y);
    r = a * b;
  } else {
    panic("Not Implemented");
  }
  return r;
}

static double flt_div(Object *x, Object *y)
{
  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = a / b;
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = a / b;
  } else if (float_check(y)) {
    double b = float_asflt(y);
    r = a / b;
  } else {
    panic("Not Implemented");
  }
  return r;
}

static double flt_mod(Object *x, Object *y)
{
  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = fmod(a, b);
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = fmod(a, b);
  } else if (float_check(y)) {
    double b = float_asflt(y);
    r = fmod(a, b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static int64_t flt_pow(Object *x, Object *y)
{
  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = pow(a, b);
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = pow(a, b);
  } else if (float_check(y)) {
    double b = float_asflt(y);
    r = pow(a, b);
  } else {
    panic("Not Implemented");
  }
  return r;
}

static Object *flt_num_add(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__add__\" must be two operands");
    return NULL;
  }

  return float_new(flt_add(x, y));
}

static Object *flt_num_sub(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__sub__\" must be two operands");
    return NULL;
  }

  return float_new(flt_sub(x, y));
}

static Object *flt_num_mul(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__mul__\" must be two operands");
    return NULL;
  }

  return float_new(flt_mul(x, y));
}

static Object *flt_num_div(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__div__\" must be two operands");
    return NULL;
  }

  return float_new(flt_div(x, y));
}

static Object *flt_num_mod(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__mod__\" must be two operands");
    return NULL;
  }

  return float_new(flt_mod(x, y));
}

static Object *flt_num_pow(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y == NULL) {
    error("\"__pow__\" must be two operands");
    return NULL;
  }

  return float_new(flt_pow(x, y));
}

static Object *flt_num_neg(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    return NULL;
  }

  if (y != NULL) {
    error("'-' must be only one operand");
    return NULL;
  }

  Object *z;
  double a = float_asflt(x);
  z = float_new((double)0 - a);
  return z;
}

static double flt_num_cmp(Object *x, Object *y)
{
  if (!float_check(x)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(x));
    //FIXME: retval?
    return 0;
  }

  double a = float_asflt(x);
  double r;
  if (integer_check(y)) {
    int64_t b = integer_asint(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %"PRId64" = %lf", a, b, r);
  } else if (byte_check(y)) {
    int b = byte_asint(y);
    r = a - b;
    if ((a > 0 && b < 0 && r < 0) ||
        (a < 0 && b > 0 && r > 0))
      panic("overflow:%lf + %d = %lf", a, b, r);
  } else if (float_check(y)) {
    double b = float_asflt(y);
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
  return (r > 0) ? bool_true() : bool_false();
}

static Object *flt_num_ge(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r >= 0) ? bool_true() : bool_false();
}

static Object *flt_num_lt(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r < 0) ? bool_true() : bool_false();
}

static Object *flt_num_le(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r <= 0) ? bool_true() : bool_false();
}

static Object *flt_num_eq(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r == 0) ? bool_true() : bool_false();
}

static Object *flt_num_neq(Object *x, Object *y)
{
  double r = flt_num_cmp(x, y);
  return (r != 0) ? bool_true() : bool_false();
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

TypeObject float_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Float",
  .free    = float_free,
  .str     = float_str,
  .number  = &float_numbers,
};

void init_float_type(void)
{
  float_type.desc = desc_from_float;
  type_add_base(&float_type, &number_type);
  if (type_ready(&float_type) < 0)
    panic("Cannot initalize 'Float' type.");
}

Object *float_new(double val)
{
  FloatObject *f = kmalloc(sizeof(FloatObject));
  init_object_head(f, &float_type);
  f->value = val;
  return (Object *)f;
}
