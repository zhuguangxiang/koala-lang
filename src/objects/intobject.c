/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/intobject.h"

TypeObject int_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Integer",
};

TypeObject bool_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Bool",
};

TypeObject byte_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Byte",
};

Object *_int_add_(Object *ob, Object *args)
{
  return NULL;
}

static struct cfuncdef int_funcs[] = {
  {"__add__", "i", "i", _int_add_},
  {NULL}
};

BoolObject true_object;
BoolObject false_object;

void init_intobject(void)
{
  mtable_init(&int_type.mtbl);
  klass_add_cfuncs(&int_type, int_funcs);
}

void fini_intobject(void)
{

}

Object *new_integer(int64_t val)
{
  return NULL;
}

Object *new_byte(int val)
{
  return NULL;
}
