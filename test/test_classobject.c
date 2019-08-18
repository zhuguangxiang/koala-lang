/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  Koala_Initialize();

  Object *s = String_New("Hello, Koala");
  Object *clazz = Object_GetValue(s, "__class__");
  assert(clazz);
  assert(((ClassObject *)clazz)->obj == s);
  OB_DECREF(s);
  OB_DECREF(clazz);

  clazz = Class_New((Object *)&String_Type);
  assert(((ClassObject *)clazz)->obj == (Object *)&String_Type);
  Object *v = Object_GetValue(clazz, "__name__");
  assert(!strcmp("String", String_AsStr(v)));
  OB_DECREF(v);

  v = Object_GetValue(clazz, "__class__");
  assert(v);
  assert(((ClassObject *)v)->obj == clazz);
  OB_DECREF(v);

  v = Object_GetValue(clazz, "__module__");
  assert(Module_Check(v));
  OB_DECREF(v);
  v = Object_GetValue(v, "__name__");
  assert(!strcmp("lang", String_AsStr(v)));
  OB_DECREF(v);

  s = String_New("length");
  v = Object_Call(clazz, "getMethod", s);
  OB_DECREF(clazz);
  OB_DECREF(s);

  assert(Method_Check(v));
  s = String_New("Hello, Koala");
  Object *res = Method_Call(v, s, NULL);
  assert(Integer_Check(res));
  assert(12 == Integer_AsInt(res));
  OB_DECREF(v);
  OB_DECREF(res);
  OB_DECREF(s);

  Koala_Finalize();
  return 0;
}
