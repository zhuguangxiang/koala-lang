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

#include "koala/moduleobject.h"

static Object *test_display(Object *self, Object *ob)
{
  printf("test_display()\n");
  return NULL;
}

static MethodDef testmodule_funcs[] = {
  {"display", NULL, NULL, test_display},
  {NULL},
};

void init_module(Object *self)
{
  expect(module_check(self));
  ModuleObject *mo = (ModuleObject *)self;
  printf("init_module(): testmodule\n");
  printf("module-name: %s\n", mo->name);
  module_add_funcdefs(self, testmodule_funcs);
}

void fini_module(Object *self)
{
  expect(module_check(self));
  ModuleObject *mo = (ModuleObject *)self;
  printf("fini_module(): testmodule\n");
  printf("module-name: %s\n", mo->name);
  module_uninstall("testmodule");
}

// gcc testmodule/testmodule.c -fPIC -shared -o libtestmodule.so
