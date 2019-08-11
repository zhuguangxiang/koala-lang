/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

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
  Koala_Initialize();

  Object *m = Module_New("test");

  Object *v = Object_GetValue(m, "__name__");
  assert(!strcmp("test", String_AsStr(v)));

  Object *clazz = Object_GetValue(m, "__class__");
  assert(Class_Check(clazz));
  ClassObject *cls = (ClassObject *)clazz;
  assert(cls->obj == m);

  v = Object_GetValue(clazz, "__name__");
  assert(!strcmp("Module", String_AsStr(v)));

  v = Object_GetValue(clazz, "__module__");
  v = Object_GetValue(v, "__name__");
  assert(!strcmp("lang", String_AsStr(v)));

  Object *meth = Object_Call(clazz, "getMethod", String_New("__name__"));
  assert(Method_Check(meth));
  v = Object_Call(meth, "call", m);
  assert(!strcmp("test", String_AsStr(v)));

  v = Method_Call(meth, m, NULL);
  assert(!strcmp("test", String_AsStr(v)));

  Koala_Finalize();
  return 0;
}
