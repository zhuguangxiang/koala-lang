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

static Object *assert_equal(Object *self, Object *arg)
{
  extern int halt;
  int failed = 0;
  expect(module_check(self));
  expect(tuple_check(arg));
  expect(tuple_size(arg) == 2);

  Object *ob1 = tuple_get(arg, 0);
  Object *ob2 = tuple_get(arg, 1);

  if (ob1 != ob2) {
    Object *res = OB_TYPE(ob1)->equal(ob1, ob2);
    failed = bool_isfalse(res);
    OB_DECREF(res);
  }

  OB_DECREF(ob1);
  OB_DECREF(ob2);

  if (failed) {
    error("assert.equal failed\n");
    halt = 1;
  }

  return NULL;
}

static MethodDef assertfuncs[] = {
  {"equal", "AA", NULL, assert_equal},
  {NULL}
};

void init_assert_module(void)
{
  Object *m = module_new("assert");
  module_add_funcdefs(m, assertfuncs);
  module_install("assert", m);
  OB_DECREF(m);
}

void fini_assert_module(void)
{
  module_uninstall("assert");
}
