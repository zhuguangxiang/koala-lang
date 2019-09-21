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
#include "log.h"

int main(int argc, char *argv[])
{
  koala_initialize();

  Object *s = string_new("Hello, Koala");
  Object *clazz = Object_GetValue(s, "__class__");
  expect(clazz);
  expect(((ClassObject *)clazz)->obj == s);
  OB_DECREF(s);
  OB_DECREF(clazz);

  clazz = Class_New((Object *)&string_type);
  expect(((ClassObject *)clazz)->obj == (Object *)&string_type);
  Object *v = Object_GetValue(clazz, "__name__");
  expect(!strcmp("String", string_asstr(v)));
  OB_DECREF(v);

  v = Object_GetValue(clazz, "__class__");
  expect(v);
  expect(((ClassObject *)v)->obj == clazz);
  OB_DECREF(v);

  v = Object_GetValue(clazz, "__module__");
  expect(Module_Check(v));
  OB_DECREF(v);
  v = Object_GetValue(v, "__name__");
  expect(!strcmp("lang", string_asstr(v)));
  OB_DECREF(v);

  s = string_new("length");
  v = Object_Call(clazz, "getMethod", s);
  OB_DECREF(clazz);
  OB_DECREF(s);

  expect(Method_Check(v));
  s = string_new("Hello, Koala");
  Object *res = Method_Call(v, s, NULL);
  expect(integer_check(res));
  expect(12 == integer_asint(res));
  OB_DECREF(v);
  OB_DECREF(res);
  OB_DECREF(s);

  koala_finalize();
  return 0;
}
