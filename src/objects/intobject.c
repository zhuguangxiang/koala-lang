/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/intobject.h"

struct klass int_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Integer",
};

struct klass bool_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Bool",
};

struct klass byte_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Byte",
};

struct object *__int_add__(struct object *ob, struct object *args)
{
  return NULL;
}

static struct cfuncdef int_funcs[] = {
  {"__add__", "i", "i", __int_add__},
  {NULL}
};

struct bool_object true_obj;
struct bool_object false_obj;

void init_int_type(void)
{
  mtable_init(&int_type.mtbl);
  klass_add_cfuncs(&int_type, int_funcs);
}

void free_int_type(void)
{

}

struct object *new_integer(int64_t val)
{
  return NULL;
}

struct object *new_byte(int val)
{
  return NULL;
}
