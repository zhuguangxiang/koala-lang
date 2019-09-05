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
  if (!Array_Check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  if (arr->type == NULL) {
    arr->type = OB_INCREF(OB_TYPE(val));
  } else {
    if (arr->type != OB_TYPE(val)) {
      error("expected object of '%.64s', but '%.64s'",
            arr->type->name, OB_TYPE_NAME(val));
      return NULL;
    }
  }
  vector_push_back(&arr->items, OB_INCREF(val));
  return NULL;
}

static Object *array_pop(Object *self, Object *args)
{
  if (!Array_Check(self)) {
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
  if (!Array_Check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int index = Integer_AsInt(args);
  Object *val = vector_get(&arr->items, index);
  return OB_INCREF(val);
}

static Object *array_setitem(Object *self, Object *args)
{
  if (!Array_Check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Object *index = Tuple_Get(args, 0);
  Object *val = Tuple_Get(args, 1);
  Object *old = vector_get(&arr->items, Integer_AsInt(index));
  OB_DECREF(old);
  vector_set(&arr->items, Integer_AsInt(index), OB_INCREF(val));
  return NULL;
}

static Object *array_slice(Object *self, Object *args)
{

}

static Object *array_length(Object *self, Object *args)
{
  if (!Array_Check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = vector_size(&arr->items);
  return Integer_New(len);
}

static MethodDef array_methods[] = {
  {"append",  "A",  NULL, array_append },
  {"pop",     NULL, "A",  array_pop    },
  {"insert",  "iA", NULL, array_insert },
  {"remove",  "i",  "A",  array_remove },
  {"sort",    NULL, NULL, array_sort   },
  {"reverse", NULL, NULL, array_reverse},
  {"__getitem__",  "i",  "A",  array_getitem},
  {"__setitem__",  "iA",  NULL, array_setitem},
  {"__getslice__", "ii",  "[A", array_slice  },
  {"__setslice__", "iiA", NULL, array_slice  },
  {"length",       NULL,  "i",  array_length },
  {NULL}
};

static void array_item_free(void *item, void *arg)
{
  OB_DECREF(item);
}

void Array_Free(Object *ob)
{
  if (!Array_Check(ob)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(ob));
    return;
  }

  ArrayObject *arr = (ArrayObject *)ob;
  OB_DECREF(arr->type);
  vector_fini(&arr->items, array_item_free, NULL);
  kfree(arr);
}

Object *array_str(Object *self, Object *ob)
{
  if (!Array_Check(self)) {
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

TypeObject Array_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Array",
  .free    = Array_Free,
  .str     = array_str,
  .methods = array_methods,
};

Object *Array_New(void)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  Init_Object_Head(arr, &Array_Type);
  vector_init(&arr->items);
  return (Object *)arr;
}

void Array_Print(Object *ob)
{
  ArrayObject *arr = (ArrayObject *)ob;
  VECTOR_ITERATOR(iter, &arr->items);
  print("[");
  Object *item;
  Object *str;
  iter_for_each(&iter, item) {
    str = Object_Call(item, "__str__", NULL);
    print("'%s', ", String_AsStr(str));
    OB_DECREF(str);
  }
  print("]\n");
}

int Array_Set(Object *self, int index, Object *v)
{
  if (!Array_Check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return -1;
  }

  ArrayObject *arr = (ArrayObject *)self;
  vector_set(&arr->items, index, OB_INCREF(v));
  return 0;
}
