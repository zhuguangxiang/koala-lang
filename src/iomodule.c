/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"

static Object *_io_print_(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  IoPrint(args);
  return NULL;
}

static Object *_io_println_(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  IoPrintln(args);
  return NULL;
}

static MethodDef io_methods[] = {
  {"print",   "A", NULL, _io_print_   },
  {"println", "A", NULL, _io_println_ },
  {NULL}
};

void init_io_module(void)
{
  Object *m = Module_New("io");
  Module_Add_FuncDefs(m, io_methods);
  Module_Install("io", m);
  OB_DECREF(m);
}

void fini_io_module(void)
{
  Module_Uninstall("io");
}

void IoPrint(Object *ob)
{
  if (ob == NULL)
    return;

  if (String_Check(ob)) {
    printf("\"%s\"", String_AsStr(ob));
  } else {
    Object *str = Object_Call(ob, "__str__", NULL);
    if (str != NULL)
      printf("%s", String_AsStr(str));
    else
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
  }
}

void IoPrintln(Object *ob)
{
  if (ob == NULL)
    return;

  if (String_Check(ob)) {
    printf("\"%s\"\n", String_AsStr(ob));
  } else {
    Object *str = Object_Call(ob, "__str__", NULL);
    if (str != NULL)
      printf("%s\n", String_AsStr(str));
    else
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
  }
}
