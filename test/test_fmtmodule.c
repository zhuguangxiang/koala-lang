/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  koala_initialize();

  Object *m = Module_Load("fmt");
  Object *fmtstr = String_New("Hello, {}. I was born at {}.");
  Object *name = String_New("Koala");
  Object *year = Integer_New(2018);
  Object *args = Tuple_Pack(3, fmtstr, name, year);
  Object_Call(m, "println", args);
  OB_DECREF(args);
  OB_DECREF(fmtstr);

  fmtstr = String_New("{}");
  Object *tuple = Tuple_Pack(4, name, year, name, year);
  args = Tuple_Pack(2, fmtstr, tuple);
  Object_Call(m, "println", args);
  OB_DECREF(args);
  OB_DECREF(tuple);
  OB_DECREF(fmtstr);

  OB_DECREF(year);
  OB_DECREF(name);

  OB_DECREF(m);

  koala_finalize();
  return 0;
}
