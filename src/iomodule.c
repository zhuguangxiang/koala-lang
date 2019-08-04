/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "stringobject.h"
#include "log.h"

void init_io_module(void)
{

}

void io_put(Object *ob)
{
  if (String_Check(ob)) {
    printf("%s", String_AsStr(ob));
  } else {
    Object *str = Object_Call(ob, "__str__", NULL);
    if (str != NULL)
      printf("%s", String_AsStr(str));
    else
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
  }
}

void io_putln(Object *ob)
{
  if (String_Check(ob)) {
    printf("%s\n", String_AsStr(ob));
  } else {
    Object *str = Object_Call(ob, "__str__", NULL);
    if (str != NULL)
      printf("%s\n", String_AsStr(str));
    else
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
  }
}
