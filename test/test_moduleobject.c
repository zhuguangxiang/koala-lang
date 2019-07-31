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
  f := clazz.getField("__name__")
  name := f.get(m)
  println(name)

 */
int main(int argc, char *argv[])
{
  Koala_Initialize();

  Object *m = Module_New("lang");
  Object *v = Object_GetValue(m, "__name__");
  assert(!strcmp("lang", String_AsStr(v)));

  Object *clazz = Object_GetValue(m, "__class__");
  assert(Class_Check(clazz));
  ClassObject *cls = (ClassObject *)clazz;
  assert(cls->obj == m);

  Object *field = Object_CallMethod(clazz, "getField", String_New("__name__"));
  assert(Field_Check(field));
  v = Object_CallMethod(field, "get", m);
  assert(!strcmp("lang", String_AsStr(v)));

  Koala_Finalize();
  return 0;
}
