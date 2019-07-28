/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/floatobject.h"

TypeObject float_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Float",
};

static struct cfuncdef float_funcs[] = {
  {NULL}
};

void init_floatobject(void)
{
  mtbl_init(&float_type.mtbl);
  klass_add_cfuncs(&float_type, float_funcs);
}

void fini_floatobject(void)
{

}

Object *new_float(double val)
{
  return NULL;
}
