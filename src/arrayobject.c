/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "arrayobject.h"
#include "intobject.h"
#include "tupleobject.h"
#include "strbuf.h"

static Object *array_append(Object *self, Object *val)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  vector_push_back(&arr->items, OB_INCREF(val));
  return NULL;
}

static Object *array_pop(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return vector_pop_back(&arr->items);
}

static Object *array_insert(Object *self, Object *args)
{

}

static Object *array_remove(Object *self, Object *args)
{

}

static Object *array_sort(Object *self, Object *args)
{

}

static Object *array_reverse(Object *self, Object *args)
{

}

static Object *array_getitem(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int index = Integer_AsInt(args);
  int size = vector_size(&arr->items);
  if (index < 0 || index >= size) {
    error("index %d out of range(0..<%d)", index, size);
    return NULL;
  }
  Object *val = vector_get(&arr->items, index);
  return OB_INCREF(val);
}

static Object *array_setitem(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Object *index = Tuple_Get(args, 0);
  Object *val = Tuple_Get(args, 1);
  OB_INCREF(val);
  array_set(self, Integer_AsInt(index), val);
  return NULL;
}

static Object *array_length(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = vector_size(&arr->items);
  return Integer_New(len);
}

static MethodDef array_methods[] = {
  {"append",      "<T>",  NULL,   array_append  },
  {"pop",         NULL,   "<T>",  array_pop     },
  {"insert",      "i<T>", NULL,   array_insert  },
  {"remove",      "i",    "<T>",  array_remove  },
  {"sort",        NULL,   NULL,   array_sort    },
  {"reverse",     NULL,   NULL,   array_reverse },
  {"length",      NULL,   "i",    array_length  },
  {"__getitem__", "i",    "<T>",  array_getitem },
  {"__setitem__", "i<T>", NULL,   array_setitem },
  {NULL}
};

void array_free(Object *ob)
{
  if (!array_check(ob)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(ob));
    return;
  }

  ArrayObject *arr = (ArrayObject *)ob;
  TYPE_DECREF(arr->desc);
  Object *item;
  vector_for_each(item, &arr->items) {
    OB_DECREF(item);
  }
  kfree(arr);
}

Object *array_str(Object *self, Object *ob)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  STRBUF(sbuf);
  VECTOR_ITERATOR(iter, &arr->items);
  strbuf_append_char(&sbuf, '[');
  Object *str;
  Object *tmp;
  int i = 0;
  int size = vector_size(&arr->items);
  iter_for_each(&iter, tmp) {
    if (String_Check(tmp)) {
      strbuf_append_char(&sbuf, '"');
      strbuf_append(&sbuf, String_AsStr(tmp));
      strbuf_append_char(&sbuf, '"');
    } else {
      str = Object_Call(tmp, "__str__", NULL);
      strbuf_append(&sbuf, String_AsStr(str));
      OB_DECREF(str);
    }
    if (i++ < size - 1)
      strbuf_append(&sbuf, ", ");
  }
  strbuf_append(&sbuf, "]");
  str = String_New(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);

  return str;
}

TypeObject array_type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Array",
  .free    = array_free,
  .str     = array_str,
  .methods = array_methods,
};

void init_array_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Array");
  Vector *vec = vector_new();
  vector_push_back(vec, new_typeparadef("T", NULL));
  desc->typeparas = vec;
  array_type.desc = desc;
  if (type_ready(&array_type) < 0)
    panic("Cannot initalize 'Array' type.");
}

Object *array_new(TypeDesc *desc)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = TYPE_INCREF(desc);
  return (Object *)arr;
}

void Array_Print(Object *ob)
{
  ArrayObject *arr = (ArrayObject *)ob;
  print("[");
  Object *item;
  Object *str;
  vector_for_each(item, &arr->items) {
    str = Object_Call(item, "__str__", NULL);
    print("'%s', ", String_AsStr(str));
    OB_DECREF(str);
  }
  print("]\n");
}

int array_set(Object *self, int index, Object *v)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return -1;
  }

  ArrayObject *arr = (ArrayObject *)self;

  int size = vector_size(&arr->items);
  if (index < 0 || index > size) {
    error("index %d out of range(0...%d)", index, size);
    return -1;
  }

  Object *oldval = vector_set(&arr->items, index, OB_INCREF(v));
  OB_DECREF(oldval);
  return 0;
}
