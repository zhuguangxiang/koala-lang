/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/arrayobject.h"

TypeObject array_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "String",
};

static struct cfuncdef array_funcs[] = {
  {NULL}
};

void init_arrayobject(void)
{
  mtbl_init(&array_type.mtbl);
  klass_add_cfuncs(&array_type, array_funcs);
}

void fini_arrayobject(void)
{

}

Object *new_array(int dims, TypeDesc *type)
{
  return NULL;
}
