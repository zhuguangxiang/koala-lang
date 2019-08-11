/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  Koala_Initialize();
  Object *dict = Dict_New();
  Object *foo = String_New("foo");
  Dict_Put(dict, foo, Integer_New(100));
  Object *bar = String_New("foo");
  Object *ob = Dict_Get(dict, bar);
  assert(Integer_Check(ob));
  assert(100 == Integer_AsInt(ob));

  Object *clazz = Object_GetValue(dict, "__class__");
  Object *meth = Object_Call(clazz, "getMethod", String_New("__getitem__"));

  Koala_Finalize();
  return 0;
}
