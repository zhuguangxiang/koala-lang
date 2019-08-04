/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

/*
  arr := []
  arr.append("hello")
  arr.append("world")
  print(arr)
  arr.pop()
  arr.pop()
 */
int main(int argc, char *argv[])
{
  Koala_Initialize();
  Object *arr = Array_New();

  Object *v = String_New("hello");
  Object_Call(arr, "append", v);
  OB_DECREF(v);

  v = String_New("world");
  Object_Call(arr, "append", v);
  OB_DECREF(v);

  Array_Print(arr);

  Object *i = Integer_New(1);
  Object_Call(arr, "append", i);

  v = Object_Call(arr, "__getitem__", i);
  assert(!strcmp("world", String_AsStr(v)));
  OB_DECREF(i);
  OB_DECREF(v);

  v = Object_Call(arr, "pop", NULL);
  assert(!strcmp("world", String_AsStr(v)));
  OB_DECREF(v);

  v = Object_Call(arr, "pop", NULL);
  assert(!strcmp("hello", String_AsStr(v)));
  OB_DECREF(v);

  OB_DECREF(arr);
  Koala_Finalize();
  return 0;
}
