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

#include "koala.h"

static Object *_io_print_(Object *self, Object *args)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  IoPrint(args);
  return NULL;
}

static Object *_io_println_(Object *self, Object *args)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  IoPrintln(args);
  return NULL;
}

static MethodDef io_methods[] = {
  {"print",   "A", NULL, _io_print_   },
  {"println", "A", NULL, _io_println_ },
  {NULL}
};

void init_io_module(void)
{
  Object *m = module_new("io");
  module_add_funcdefs(m, io_methods);
  module_install("io", m);
  OB_DECREF(m);
}

void fini_io_module(void)
{
  module_uninstall("io");
}

void IoPrint(Object *ob)
{
  if (ob == NULL)
    return;
  Object *v = tuple_get(ob, 0);
  if (string_check(v)) {
    printf("%s", string_asstr(v));
  } else if (integer_check(v)) {
    printf("%ld", integer_asint(v));
  } else if (byte_check(v)) {
    printf("%d", byte_asint(v));
  } else if (float_check(v)) {
    printf("%lf", float_asflt(v));
  } else {
    Object *str = object_call(v, "__str__", NULL);
    if (str != NULL) {
      printf("%s", string_asstr(str));
      OB_DECREF(str);
    } else {
      error("object of '%.64s' is not printable", OB_TYPE_NAME(v));
    }
  }
  OB_DECREF(v);
  fflush(stdout);
}

void IoPrintln(Object *ob)
{
  if (ob == NULL)
    return;
  Object *v = tuple_get(ob, 0);
  if (string_check(v)) {
    printf("%s\n", string_asstr(v));
  } else if (integer_check(v)) {
    printf("%ld\n", integer_asint(v));
  } else if (byte_check(v)) {
    printf("%d\n", byte_asint(v));
  } else if (float_check(v)) {
    printf("%lf\n", float_asflt(v));
  } else {
    Object *str = object_call(v, "__str__", NULL);
    if (str != NULL) {
      printf("%s\n", string_asstr(str));
      OB_DECREF(str);
    } else {
      error("object of '%.64s' is not printable", OB_TYPE_NAME(v));
    }
  }
  OB_DECREF(v);
  fflush(stdout);
}
