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

/*
  m := __import__("test")
  println(m.__name__)

  clazz := m.__class__
  println(clazz.__name__)

  meth := clazz.get("__name__")
  name := meth.call(m)
  println(name)

 */
int main(int argc, char *argv[])
{
  koala_initialize();

  Object *m = Module_New("test");

  Object *v = Object_GetValue(m, "__name__");
  expect(!strcmp("test", String_AsStr(v)));
  OB_DECREF(v);

  Object *clazz = Object_GetValue(m, "__class__");
  expect(Class_Check(clazz));
  ClassObject *cls = (ClassObject *)clazz;
  expect(cls->obj == m);

  v = Object_GetValue(clazz, "__name__");
  expect(!strcmp("Module", String_AsStr(v)));
  OB_DECREF(v);

  Object *mo = Object_GetValue(clazz, "__module__");
  v = Object_GetValue(mo, "__name__");
  expect(!strcmp("lang", String_AsStr(v)));
  OB_DECREF(v);
  OB_DECREF(mo);

  Object *s = string_new("__name__");
  Object *meth = Object_Call(clazz, "getMethod", s);
  expect(Method_Check(meth));
  OB_DECREF(s);
  v = Method_Call(meth, m, NULL);
  expect(!strcmp("test", String_AsStr(v)));
  OB_DECREF(meth);
  OB_DECREF(v);
  OB_DECREF(clazz);

  OB_DECREF(m);

  koala_finalize();
  return 0;
}
