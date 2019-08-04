/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "intobject.h"
#include "stringobject.h"
#include "fmtmodule.h"

static void integer_free(Object *ob)
{
  if (!Integer_Check(ob)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(ob));
    return;
  }
  debug("Integer %ld freed", Integer_AsInt(ob));
  kfree(ob);
}

static Object *integer_str(Object *self, Object *ob)
{
  if (!Integer_Check(self)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(self));
    return NULL;
  }

  IntegerObject *i = (IntegerObject *)self;
  char buf[256];
  sprintf(buf, "%ld", i->value);
  return String_New(buf);
}

static Object *integer_fmt(Object *self, Object *ob)
{
  if (!Integer_Check(self)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(self));
    return NULL;
  }

  Fmtter_WriteInteger(ob, self);
  return NULL;
}

static MethodDef int_methods[]= {
  {"__fmt__", "Llang.Formatter;", NULL, integer_fmt},
  {NULL}
};

TypeObject Integer_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Integer",
  .free    = integer_free,
  .str     = integer_str,
  .lookup  = Object_Lookup,
  .methods = int_methods,
};

Object *Integer_New(int64_t val)
{
  IntegerObject *integer = kmalloc(sizeof(*integer));
  Init_Object_Head(integer, &Integer_Type);
  integer->value = val;
  return (Object *)integer;
}

TypeObject Byte_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Byte",
};

TypeObject Bool_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Bool",
};

BoolObject OB_True = {
  OBJECT_HEAD_INIT(&Bool_Type)
  .value = 1,
};

BoolObject OB_False = {
  OBJECT_HEAD_INIT(&Bool_Type)
  .value = 0,
};
