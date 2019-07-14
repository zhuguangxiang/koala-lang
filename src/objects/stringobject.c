/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/stringobject.h"

TypeObject string_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "String",
};

Object *_string_concat_(Object *ob, Object *args)
{
  OB_TYPE_ASSERT(ob, &string_type);
  OB_TYPE_ASSERT(args, &string_type);
  return NULL;
}

Object *_string_length_(Object *ob, Object *args)
{
  OB_TYPE_ASSERT(ob, &string_type);
  assert(!args);
  return NULL;
}

static struct cfuncdef str_funcs[] = {
  {"concat", "s", "s", _string_concat_},
  {"length", NULL, "i", _string_length_},
  {"__add__", "s", "s", _string_concat_},
  {NULL}
};

void init_stringobject(void)
{
  mtable_init(&string_type.mtbl);
  klass_add_cfuncs(&string_type, str_funcs);
}

void fini_stringobject(void)
{

}

Object *new_string(char *str)
{
  return NULL;
}
