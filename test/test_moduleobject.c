/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

/*
  m := load_module("lang")
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

  Object *v = Object_Get(m, "__name__");
  assert(!strcmp("test", String_AsStr(v)));

  Object *clazz = Object_Get(m, "__class__");
  assert(Class_Check(clazz));
  ClassObject *cls = (ClassObject *)clazz;
  assert(cls->obj == m);

  v = Object_Get(clazz, "__name__");
  assert(!strcmp("Module", String_AsStr(v)));

  v = Object_Get(clazz, "__module__");
  assert(v == m);

  Object *meth = Object_Call(clazz, "get_method", String_New("__name__"));
  assert(Method_Check(meth));
  v = Method_Call(meth, m, NULL);
  assert(!strcmp("test", String_AsStr(v)));

  Koala_Finalize();
  return 0;
}
