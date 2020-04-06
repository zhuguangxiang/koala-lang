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

static Object *hello(Object *self, Object *ob)
{
  return string_new("hello, native extension");
}

static MethodDef testex_funcs[] = {
  {"hello", NULL, "s", hello},
  {NULL},
};

void init_module(void *ptr)
{
  Object *mo = ptr;
  printf("init_module(): testex\n");
  module_add_funcdefs(mo, testex_funcs);
  Object *var = module_get(mo, "greeting");
  Object *s = string_new("greeting from native extension");
  field_set(var, mo, s);
  OB_DECREF(var);
  OB_DECREF(s);
}

void fini_module(void *ptr)
{
  printf("fini_module(): testex\n");
}
