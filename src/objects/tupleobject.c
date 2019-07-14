/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdarg.h>
#include "objects/tupleobject.h"

struct klass tuple_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Tuple",
};

struct object *__tuple_get__(struct object *ob, struct object *args)
{

}

struct object *__tuple_length__(struct object *ob, struct object *args)
{

}

static struct cfuncdef tuple_funcs[] = {
  {"get", "i", "A", __tuple_get__},
  {"length", NULL, "i", __tuple_length__},
  {NULL}
};

void init_tuple_type(void)
{
  mtable_init(&tuple_type.mtbl);
  klass_add_cfuncs(&tuple_type, tuple_funcs);
}

void free_tuple_type(void)
{

}

struct object *new_tuple(int size)
{
  int sz = sizeof(struct tuple_object) + sizeof(struct object *) * size;
  struct tuple_object *tuple = kmalloc(sz);
  init_object_head(tuple, &tuple_type);
  tuple->size = size;
  return (struct object *)tuple;
}

struct object *new_vtuple(int size, va_list ap)
{
  struct object *item;
  struct tuple_object *tuple = (struct tuple_object *)new_tuple(size);
  for (int i = 0; i < size; i++) {
    item = va_arg(ap, struct object *);
    tuple->items[i] = item;
  }
  return (struct object *)tuple;
}

struct object *new_ntuple(int size, ...)
{
  struct object *ob;
  va_list ap;
  va_start(ap, size);
  ob = new_vtuple(size, ap);
  va_end(ap);
  return ob;
}
