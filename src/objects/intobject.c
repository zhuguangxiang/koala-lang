/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "intobject.h"

TypeObject Integer_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Integer",
};

TypeObject Bool_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Bool",
};

TypeObject Byte_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Byte",
};

BoolObject True_Object;
BoolObject False_Object;

Object *Integer_New(int64_t val)
{
  IntegerObject *integer = kmalloc(sizeof(*integer));
  Init_Object_Head(integer, &Integer_Type);
  integer->value = val;
  return (Object *)integer;
}
