/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdarg.h>
#include "tupleobject.h"
#include "fmtmodule.h"
#include "log.h"

#define MSIZE(size) \
  (sizeof(TupleObject) + sizeof(void *) * (size))

Object *Tuple_New(int size)
{
  TupleObject *tuple = kmalloc(MSIZE(size));
  Init_Object_Head(tuple, &Tuple_Type);
  tuple->size = size;
  return (Object *)tuple;
}

void Tuple_Free(Object *ob)
{
  if (!Tuple_Check(ob)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(ob));
    return;
  }
  int size = Tuple_Size(ob);
  TupleObject *tuple = (TupleObject *)ob;
  Object *item;
  for (int i = 0; i < size; ++i) {
    item = tuple->items[i];
    OB_DECREF(item);
  }
  kfree(ob);
}

Object *Tuple_Pack(int size, ...)
{
  Object *ob = Tuple_New(size);
  va_list ap;

  va_start(ap, size);
  Object *item;
  TupleObject *tuple = (TupleObject *)ob;
  for (int i = 0; i < size; ++i) {
    item = va_arg(ap, Object *);
    tuple->items[i] = OB_INCREF(item);
  }
  va_end(ap);

  return ob;
}

int Tuple_Size(Object *self)
{
  if (self == NULL) {
    warn("tuple pointer is null.");
    return -1;
  }

  if (!Tuple_Check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return -1;
  }
  return ((TupleObject *)self)->size;
}

Object *Tuple_Get(Object *self, int index)
{
  if (!Tuple_Check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  TupleObject *tuple = (TupleObject *)self;
  if (index < 0 || index > tuple->size) {
    error("tuple index out of range.");
    return NULL;
  }

  Object *ob = tuple->items[index];
  return OB_INCREF(ob);
}

int Tuple_Set(Object *self, int index, Object *val)
{
  if (!Tuple_Check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return -1;
  }

  TupleObject *tuple = (TupleObject *)self;
  if (index < 0 || index > tuple->size) {
    error("tuple index out of range.");
    return -1;
  }

  tuple->items[index] = OB_INCREF(val);
  return 0;
}

/* [i ..< j] */
Object *Tuple_Slice(Object *self, int i, int j)
{
  if (!Tuple_Check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  int size = Tuple_Size(self);
  if (i < 0)
    i = 0;
  if (j < 0 || j > size)
    j = size;
  if (j < i)
    j = i;

  if (i == 0 && j == size) {
    return OB_INCREF(self);
  }

  int len = j - i;
  TupleObject *tuple = (TupleObject *)Tuple_New(len);
  Object **src = ((TupleObject *)self)->items + i;
  Object **dest = tuple->items;
  for (int k = 0; k < len; ++k) {
    dest[k] = OB_INCREF(src[k]);
  }
  return (Object *)tuple;
}

static Object *tuple_getitem(Object *self, Object *args)
{
  return NULL;
}

static Object *tuple_setitem(Object *self, Object *args)
{
  return 0;
}

static Object *tuple_length(Object *self, Object *args)
{
  return NULL;
}

static Object *tuple_fmt(Object *self, Object *args)
{
  if (!Tuple_Check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!Fmtter_Check(args)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(args));
    return NULL;
  }

  Fmtter_WriteTuple(args, self);
  return NULL;
}

static MethodDef tuple_methods[] = {
  {"length",      NULL, "i",  tuple_length },
  {"__getitem__", "i",  "A",  tuple_getitem},
  {"__setitem__", "iA", NULL, tuple_setitem},
  {"__fmt__",     "Llang.Formatter;", NULL, tuple_fmt},
  {NULL}
};

TypeObject Tuple_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Tuple",
  .free    = Tuple_Free,
  .methods = tuple_methods,
};

void *tuple_iter_next(struct iterator *iter)
{
  TupleObject *tuple = iter->iterable;
  if (tuple->size <= 0)
    return NULL;

  if (iter->index < tuple->size) {
    iter->item = Tuple_Get((Object *)tuple, iter->index);
    ++iter->index;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}
