/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "arrayobject.h"
#include "intobject.h"
#include "tupleobject.h"
#include "iterobject.h"
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
  int index = integer_asint(args);
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
  array_set(self, integer_asint(index), val);
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
  return integer_new(len);
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
  vector_fini(&arr->items);
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
  strbuf_append_char(&sbuf, '[');
  Object *str;
  Object *tmp;
  int i = 0;
  int size = vector_size(&arr->items);
  vector_for_each(tmp, &arr->items) {
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
  str = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);

  return str;
}

static Object *array_iter(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *idx = integer_new(0);
  Object *ret = iter_new(self, idx);
  OB_DECREF(idx);
  return ret;
}

static Object *array_iter_next(Object *iter, Object *args)
{
  if (!iter_check(iter)) {
    error("object of '%.64s' is not an Iterator", OB_TYPE_NAME(iter));
    return NULL;
  }

  Object *ob = iter_obj(iter);
  Object *idx = iter_args(iter);
  ArrayObject *arr = (ArrayObject *)ob;
  int64_t index = integer_asint(idx);

  Object *ret = NULL;
  if (index < vector_size(&arr->items)) {
    ret = array_getitem(ob, idx);
    integer_setint(idx, ++index);
  }

  return ret;
}

TypeObject array_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Array",
  .free    = array_free,
  .str     = array_str,
  .iter    = array_iter,
  .iternext = array_iter_next,
  .methods  = array_methods,
};

void init_array_type(void)
{
  TypeDesc *desc = desc_from_array;
  TypeDesc *para = desc_from_paradef("T", NULL);
  desc_add_paradef(desc, para);
  TYPE_DECREF(para);
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
