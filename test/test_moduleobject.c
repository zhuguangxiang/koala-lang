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
  OB_DECREF(v);

  Object *clazz = Object_GetValue(m, "__class__");
  assert(Class_Check(clazz));
  ClassObject *cls = (ClassObject *)clazz;
  assert(cls->obj == m);

  v = Object_GetValue(clazz, "__name__");
  assert(!strcmp("Module", String_AsStr(v)));
  OB_DECREF(v);

  Object *mo = Object_GetValue(clazz, "__module__");
  v = Object_GetValue(mo, "__name__");
  assert(!strcmp("lang", String_AsStr(v)));
  OB_DECREF(v);
  OB_DECREF(mo);

  Object *s = String_New("__name__");
  Object *meth = Object_Call(clazz, "getMethod", s);
  assert(Method_Check(meth));
  OB_DECREF(s);
  v = Method_Call(meth, m, NULL);
  assert(!strcmp("test", String_AsStr(v)));
  OB_DECREF(meth);
  OB_DECREF(v);
  OB_DECREF(clazz);

  OB_DECREF(m);

  Koala_Finalize();
  return 0;
}
