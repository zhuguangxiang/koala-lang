/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include <assert.h>

int main(int argc, char *argv[])
{
  koala_initialize();
  TypeDesc *kdesc = desc_from_str;
  TypeDesc *vdesc = desc_from_int;
  Object *dict = map_new(kdesc, vdesc);
  TYPE_DECREF(kdesc);
  TYPE_DECREF(vdesc);

  Object *foo = String_New("foo");
  Object *val = Integer_New(100);
  map_put(dict, foo, val);
  OB_DECREF(val);
  OB_DECREF(foo);

  Object *bar = String_New("foo");
  val = map_get(dict, bar);
  assert(Integer_Check(val));
  assert(100 == Integer_AsInt(val));
  OB_DECREF(val);

  val = Integer_New(200);
  map_put(dict, foo, val);
  OB_DECREF(val);

  val = map_get(dict, bar);
  assert(Integer_Check(val));
  assert(200 == Integer_AsInt(val));
  OB_DECREF(val);

  OB_DECREF(bar);
  OB_DECREF(dict);

  koala_finalize();
  return 0;
}
