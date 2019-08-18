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
  Object *val = Integer_New(100);
  Dict_Put(dict, foo, val);
  OB_DECREF(val);
  OB_DECREF(foo);

  Object *bar = String_New("foo");
  val = Dict_Get(dict, bar);
  OB_DECREF(bar);
  assert(Integer_Check(val));
  assert(100 == Integer_AsInt(val));
  OB_DECREF(val);

  OB_DECREF(dict);

  Koala_Finalize();
  return 0;
}
