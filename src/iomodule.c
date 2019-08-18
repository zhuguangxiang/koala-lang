/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "iomodule.h"
#include "moduleobject.h"
#include "stringobject.h"
#include "log.h"

static Object *_io_put_(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  io_put(args);
  return NULL;
}

static Object *_io_putln_(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  io_putln(args);
  return NULL;
}

static MethodDef io_methods[] = {
  {"put",   "A", NULL, _io_put_   },
  {"putln", "A", NULL, _io_putln_ },
  {NULL}
};

void init_io_module(void)
{
  Object *m = Module_New("io");
  Module_Add_FuncDefs(m, io_methods);
  Module_Install("io", m);
}

void io_put(Object *ob)
{
  if (String_Check(ob)) {
    printf("'%s'", String_AsStr(ob));
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
    printf("'%s'\n", String_AsStr(ob));
  } else {
    Object *str = Object_Call(ob, "__str__", NULL);
    if (str != NULL)
      printf("%s\n", String_AsStr(str));
    else
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
  }
}
