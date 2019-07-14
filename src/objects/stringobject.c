/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/stringobject.h"

struct klass string_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "String",
};

struct object *__string_concat__(struct object *ob, struct object *args)
{
  OB_TYPE_ASSERT(ob, &string_type);
  OB_TYPE_ASSERT(args, &string_type);
  return NULL;
}

struct object *__string_length__(struct object *ob, struct object *args)
{
  OB_TYPE_ASSERT(ob, &string_type);
  assert(!args);
  return NULL;
}

static struct cfuncdef str_funcs[] = {
  {"concat", "s", "s", __string_concat__},
  {"length", NULL, "i", __string_length__},
  {"__add__", "s", "s", __string_concat__},
  {NULL}
};

void init_string_type(void)
{
  mtable_init(&string_type.mtbl);
  klass_add_cfuncs(&string_type, str_funcs);
}

void free_string_type(void)
{

}

struct object *new_string(char *str)
{
  return NULL;
}
