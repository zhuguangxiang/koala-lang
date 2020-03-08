
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

#include "enumobject.h"
#include "intobject.h"
#include "tupleobject.h"

TypeObject *result_type;

#define result_check(ob) (OB_TYPE(ob) == result_type)

#define ok_str "Ok"
#define err_str "Err"

static Object *result_isok(Object *self, Object *arg)
{
  if (!result_check(self)) {
    error("object of '%.64s' is not a Result", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, ok_str)) {
    return bool_true();
  } else {
    expect(!strcmp(eob->name, err_str));
    return bool_false();
  }
}

static Object *result_iserr(Object *self, Object *arg)
{
  if (!result_check(self)) {
    error("object of '%.64s' is not a Result", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, err_str)) {
    return bool_true();
  } else {
    expect(!strcmp(eob->name, ok_str));
    return bool_false();
  }
}

Object *result_get_ok(Object *self, Object *arg)
{
  if (!result_check(self)) {
    error("object of '%.64s' is not a Result", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  expect(!strcmp(eob->name, ok_str));
  return enum_get_value(self, 0);
}

Object *result_get_err(Object *self, Object *arg)
{
  if (!result_check(self)) {
    error("object of '%.64s' is not a Result", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  expect(!strcmp(eob->name, err_str));
  return enum_get_value(self, 0);
}

static MethodDef result_methods[] = {
  {"is_ok",  NULL, "z",   result_isok },
  {"is_err", NULL, "z",   result_iserr},
  {"ok",     NULL, "<T>", result_get_ok },
  {"err",    NULL, "<E>", result_get_err},
  {NULL}
};

void init_result_type(void)
{
  result_type = enum_type_new("lang", "Result");
  type_add_tp(result_type, "T", NULL);
  type_add_tp(result_type, "E", NULL);

  TypeDesc *pref;
  VECTOR(prefs);

  pref = desc_from_pararef("T", 0);
  vector_push_back(&prefs, pref);
  enum_add_label(result_type, ok_str, &prefs);
  TYPE_DECREF(pref);
  vector_clear(&prefs);

  pref = desc_from_pararef("E", 1);
  vector_push_back(&prefs, pref);
  enum_add_label(result_type, err_str, &prefs);
  TYPE_DECREF(pref);
  vector_fini(&prefs);

  type_add_methoddefs(result_type, result_methods);

  if (type_ready(result_type) < 0)
    panic("Cannot initalize 'Result' type.");
}

void fini_result_type(void)
{
  OB_DECREF(result_type);
}

Object *result_new(int ok, Object *val)
{
  Object *args = tuple_new(1);
  tuple_set(args, 0, val);
  Object *res = enum_new((Object *)result_type, ok ? ok_str : err_str, args);
  OB_DECREF(args);
  return res;
}

int result_test(Object *self)
{
  if (!result_check(self)) {
    error("object of '%.64s' is not a Result", OB_TYPE_NAME(self));
    return 0;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, ok_str)) {
    return 1;
  } else {
    expect(!strcmp(eob->name, err_str));
    return 0;
  }
}
