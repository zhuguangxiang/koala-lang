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

  clazz = Class_New((Object *)&String_Type);
  assert(((ClassObject *)clazz)->obj == (Object *)&String_Type);
  Object *v = Object_GetValue(clazz, "__name__");
  assert(!strcmp("String", String_AsStr(v)));

  v = Object_GetValue(clazz, "__class__");
  assert(v);
  assert(((ClassObject *)v)->obj == clazz);

  v = Object_GetValue(clazz, "__module__");
  assert(Module_Check(v));
  v = Object_GetValue(v, "__name__");
  assert(!strcmp("lang", String_AsStr(v)));

  v = Object_Call(clazz, "getMethod", String_New("length"));
  assert(Method_Check(v));
  v = Method_Call(v, s, NULL);
  assert(Integer_Check(v));
  assert(12 == Integer_AsInt(v));

  Koala_Finalize();
  return 0;
}
