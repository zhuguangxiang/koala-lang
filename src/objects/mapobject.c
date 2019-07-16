/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/mapobject.h"

Klass map_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Map",
};

static struct cfuncdef map_funcs[] = {
  {NULL}
};

void init_mapobject(void)
{
  mtbl_init(&map_type.mtbl);
  klass_add_cfuncs(&map_type, map_funcs);
}

void fini_mapobject(void)
{

}

Object *new_map(void)
{
  return NULL;
}
