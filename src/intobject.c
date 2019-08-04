/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "intobject.h"

static void integer_free(Object *ob)
{
  if (!Integer_Check(ob)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(ob));
    return;
  }
  debug("Integer %ld freed", Integer_AsInt(ob));
  kfree(ob);
}

TypeObject Integer_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Integer",
  .free = integer_free,
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
