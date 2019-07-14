/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdarg.h>
#include "objects/tupleobject.h"

TypeObject tuple_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Tuple",
};

Object *_tuple_get_(Object *ob, Object *args)
{

}

Object *_tuple_length_(Object *ob, Object *args)
{

}

static struct cfuncdef tuple_funcs[] = {
  {"get", "i", "A", _tuple_get_},
  {"length", NULL, "i", _tuple_length_},
  {NULL}
};

void init_tupleobject(void)
{
  mtable_init(&tuple_type.mtbl);
  klass_add_cfuncs(&tuple_type, tuple_funcs);
}

void fini_tupleobject(void)
{

}

Object *new_tuple(int size)
{
  int sz = sizeof(TupleObject) + sizeof(Object *) * size;
  TupleObject *tuple = kmalloc(sz);
  init_object_head(tuple, &tuple_type);
  tuple->size = size;
  return (Object *)tuple;
}

static Object *new_vntuple(int size, va_list ap)
{
  Object *item;
  TupleObject *tuple = (TupleObject *)new_tuple(size);
  for (int i = 0; i < size; i++) {
    item = va_arg(ap, Object *);
    tuple->items[i] = item;
  }
  return (Object *)tuple;
}

Object *new_vtuple(int size, ...)
{
  Object *ob;
  va_list ap;
  va_start(ap, size);
  ob = new_vntuple(size, ap);
  va_end(ap);
  return ob;
}
