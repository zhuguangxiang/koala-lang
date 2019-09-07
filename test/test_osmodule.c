/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  Koala_Initialize();
  Object *m = Module_Load("os");
  assert(m);
  Object *arr = Object_GetValue(m, "path");
  assert(array_check(arr));

  Object *str = String_New("~/.local/lib/koala/libs");
  Object_Call(arr, "append", str);
  OB_DECREF(str);

  str = String_New("/usr/lib/koala/libs");
  Object_Call(arr, "append", str);
  OB_DECREF(str);

  Array_Print(arr);
  OB_DECREF(arr);

  OB_DECREF(m);

  Koala_Finalize();
  return 0;
}
