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

#include "optionobject.h"
#include "enumobject.h"
#include "methodobject.h"
#include "closureobject.h"
#include "eval.h"
#include "tupleobject.h"
#include "intobject.h"

/*
  enum Option<T> {
    Some(T),
    None,

    func is_some() bool {
      res := false
      match self {
        Some(_) => { res = true;  }
        None    => { res = false; }
      }
      return res
    }

    func is_none() bool {
      !is_some()
    }

    func some() T {
      match self {
        Some(v) => { return v;  }
        None    => { return nil; }
      }
    }

    func and_then<U>(f func(v T) Option<U>) Option<U> {

    }
  }

  var opt Option<int> = Option.Some(100)
  opt.is_some()
  opt.is_none()
  opt.some()

  f := func(v int) Option<int> {
    return Some(v * 3)
  }

  opt.and_then(func(v int) Option<int> {
    return Some(v * 2)
  }).and_then(f)

 */
TypeObject *option_type;

#define option_check(ob) (OB_TYPE(ob) == option_type)

#define some_str "Some"
#define none_str "None"

static Object *option_issome(Object *self, Object *arg)
{
  if (!option_check(self)) {
    error("object of '%.64s' is not an Option", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, some_str)) {
    return bool_true();
  } else {
    expect(!strcmp(eob->name, none_str));
    return bool_false();
  }
}

static Object *option_isnone(Object *self, Object *arg)
{
  if (!option_check(self)) {
    error("object of '%.64s' is not an Option", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, none_str)) {
    return bool_true();
  } else {
    expect(!strcmp(eob->name, some_str));
    return bool_false();
  }
}

Object *option_get_some(Object *self, Object *arg)
{
  if (!option_check(self)) {
    error("object of '%.64s' is not an Option", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  expect(!strcmp(eob->name, some_str));
  return enum_get_value(self, 0);
}

static Object *option_and_then(Object *self, Object *arg)
{
  if (!option_check(self)) {
    error("object of '%.64s' is not an Option", OB_TYPE_NAME(self));
    return NULL;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, some_str)) {
    Object *val = enum_get_value(self, 0);
    Object *z;
    if (method_check(arg)) {
      debug("call method '%s'", ((MethodObject *)arg)->name);
      z = method_call(arg, NULL, val);
    } else {
      debug("call closure '%s'", ((ClosureObject *)arg)->name);
      expect(closure_check(arg));
      z = koala_evalcode(closure_getcode(arg), arg, val, arg);
    }
    OB_DECREF(val);
    return z;
  } else {
    return enum_new((Object *)option_type, none_str, NULL);
  }
}

static MethodDef option_methods[] = {
  {"is_some",   NULL,   "z",    option_issome   },
  {"is_none",   NULL,   "z",    option_isnone   },
  {"some",      NULL,   "<T>",  option_get_some },
  {"and_then",  "P<T>:Llang.Option<T>;", "Llang.Option<T>;", option_and_then},
  {NULL}
};

void init_option_type(void)
{
  option_type = enum_type_new("lang", "Option");
  type_add_tp(option_type, "T", NULL);

  TypeDesc *pref;
  VECTOR(prefs);

  pref = desc_from_pararef("T", 0);
  vector_push_back(&prefs, pref);
  enum_add_label(option_type, some_str, &prefs);
  TYPE_DECREF(pref);
  vector_fini(&prefs);

  enum_add_label(option_type, none_str, NULL);

  type_add_methoddefs(option_type, option_methods);

  if (type_ready(option_type) < 0)
    panic("Cannot initalize 'Result' type.");
}

void fini_option_type(void)
{
  OB_DECREF(option_type);
}

Object *option_new(int some, Object *val)
{
  Object *args = NULL;
  char *name;
  if (some) {
    args = tuple_new(1);
    tuple_set(args, 0, val);
    name = some_str;
  } else {
    name = none_str;
  }

  Object *res = enum_new((Object *)option_type, name, args);
  OB_DECREF(args);
  return res;
}

int option_test(Object *self)
{
  if (!option_check(self)) {
    error("object of '%.64s' is not an Option", OB_TYPE_NAME(self));
    return 0;
  }

  EnumObject *eob = (EnumObject *)self;
  if (!strcmp(eob->name, some_str)) {
    return 1;
  } else {
    expect(!strcmp(eob->name, none_str));
    return 0;
  }
}
