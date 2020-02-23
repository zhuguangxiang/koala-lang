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
#include "floatobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "iterobject.h"
#include "strbuf.h"

static int isobj(TypeDesc *desc)
{
  if (desc->kind != TYPE_BASE)
    return 1;

  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    return 0;
  case BASE_INT:
    return 0;
  case BASE_CHAR:
    return 0;
  case BASE_FLOAT:
    return 0;
  case BASE_BOOL:
    return 0;
  default:
    return 1;
  }
}

static int type_size(TypeDesc *desc)
{
  if (desc->kind != TYPE_BASE)
    return PTR_SIZE;

  int size;
  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    size = BYTE_SIZE;
    break;
  case BASE_INT:
    size = INT_SIZE;
    break;
  case BASE_CHAR:
    size = CHAR_SIZE;
    break;
  case BASE_FLOAT:
    size = FLT_SIZE;
    break;
  case BASE_BOOL:
    size = BOOL_SIZE;
    break;
  default:
    size = PTR_SIZE;
    break;
  }
  return size;
}

static void unbox(TypeDesc *desc, Object *val, RawValue *raw)
{
  if (desc->kind != TYPE_BASE) {
    raw->obj = OB_INCREF(val);
    return;
  }

  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    raw->bval = ((ByteObject *)val)->value;
    break;
  case BASE_INT:
    raw->ival = ((IntegerObject *)val)->value;
    break;
  case BASE_CHAR:
    raw->cval = ((CharObject *)val)->value;
    break;
  case BASE_FLOAT:
    raw->fval = ((FloatObject *)val)->value;
    break;
  case BASE_BOOL:
    raw->zval = ((BoolObject *)val)->value;
    break;
  default:
    raw->obj = OB_INCREF(val);
    break;
  }
}

Object *box(TypeDesc *desc, RawValue *raw, int inc)
{
  if (desc->kind != TYPE_BASE) {
    return inc ? OB_INCREF(raw->obj) : raw->obj;
  }

  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    return byte_new(raw->bval);
  case BASE_INT:
    return integer_new(raw->ival);
  case BASE_CHAR:
    return char_new(raw->cval);
  case BASE_FLOAT:
    return float_new(raw->fval);
  case BASE_BOOL:
    return raw->zval ? bool_true() : bool_false();
  default:
    return inc ? OB_INCREF(raw->obj) : raw->obj;
  }
}

static void byte_tostr(ArrayObject *arr, StrBuf *buf)
{
  char *items = gvector_toarr(arr->vec);
  int size = gvector_size(arr->vec);
  for (int i = 0; i < size; ++i) {
    strbuf_append_int(buf, items[i]);
    if (i < size - 1)
      strbuf_append(buf, ", ");
  }
}

static void int_tostr(ArrayObject *arr, StrBuf *buf)
{
  int64_t *items = gvector_toarr(arr->vec);
  int size = gvector_size(arr->vec);
  for (int i = 0; i < size; ++i) {
    strbuf_append_int(buf, items[i]);
    if (i < size - 1)
      strbuf_append(buf, ", ");
  }
}

static void char_tostr(ArrayObject *arr, StrBuf *buf)
{
  panic("char: not implemented");
}

static void float_tostr(ArrayObject *arr, StrBuf *buf)
{
  double *items = gvector_toarr(arr->vec);
  int size = gvector_size(arr->vec);
  for (int i = 0; i < size; ++i) {
    strbuf_append_float(buf, items[i]);
    if (i < size - 1)
      strbuf_append(buf, ", ");
  }
}

static void bool_tostr(ArrayObject *arr, StrBuf *buf)
{
  int *items = gvector_toarr(arr->vec);
  int size = gvector_size(arr->vec);
  for (int i = 0; i < size; ++i) {
    strbuf_append(buf, items[i] ? "true" : "false");
    if (i < size - 1)
      strbuf_append(buf, ", ");
  }
}

static void string_tostr(ArrayObject *arr, StrBuf *buf)
{
  Object **items = gvector_toarr(arr->vec);
  int size = gvector_size(arr->vec);
  for (int i = 0; i < size; ++i) {
    strbuf_append(buf, string_asstr(items[i]));
    if (i < size - 1)
      strbuf_append(buf, ", ");
  }
}

static void obj_tostr(ArrayObject *arr, StrBuf *buf)
{
  Object **items = gvector_toarr(arr->vec);
  int size = gvector_size(arr->vec);
  for (int i = 0; i < size; ++i) {
    Object *str = object_call(items[i], "__str__", NULL);
    strbuf_append(buf, string_asstr(str));
    OB_DECREF(str);
    if (i < size - 1)
      strbuf_append(buf, ", ");
  }
}

static void tostr(ArrayObject *arr, StrBuf *buf)
{
  TypeDesc *desc = arr->desc;
  if (desc->kind != TYPE_BASE) {
    obj_tostr(arr, buf);
    return;
  }

  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    byte_tostr(arr, buf);
    break;
  case BASE_INT:
    int_tostr(arr, buf);
    break;
  case BASE_CHAR:
    char_tostr(arr, buf);
    break;
  case BASE_FLOAT:
    float_tostr(arr, buf);
    break;
  case BASE_BOOL:
    bool_tostr(arr, buf);
    break;
  case BASE_STR:
    string_tostr(arr, buf);
    break;
  default:
    obj_tostr(arr, buf);
    break;
  }
}

static GVector *get_vec(ArrayObject *arr)
{
  GVector *vec = arr->vec;
  if (vec == NULL) {
    vec = gvector_new(0, type_size(arr->desc));
    arr->vec = vec;
  }
  return vec;
}

static Object *array_append(Object *self, Object *val)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  RawValue raw;
  unbox(arr->desc, val, &raw);
  gvector_push_back(get_vec(arr), &raw);
  return NULL;
}

static Object *array_pop(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  RawValue raw;
  gvector_pop_back(get_vec(arr), &raw);
  return box(arr->desc, &raw, 0);
}

static Object *array_insert(Object *self, Object *args)
{
  return NULL;
}

static Object *array_remove(Object *self, Object *args)
{
  return NULL;
}

static Object *array_sort(Object *self, Object *args)
{
  return NULL;
}

static Object *array_reverse(Object *self, Object *args)
{
  return NULL;
}

static Object *array_getitem(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int index = integer_asint(args);
  int size = gvector_size(arr->vec);
  if (index < 0 || index >= size) {
    error("index %d out of range(0..<%d)", index, size);
    return NULL;
  }

  RawValue raw;
  gvector_get(get_vec(arr), index, &raw);
  return box(arr->desc, &raw, 1);
}

static Object *array_setitem(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *index = tuple_get(args, 0);
  Object *val = tuple_get(args, 1);
  array_set(self, integer_asint(index), val);
  OB_DECREF(index);
  OB_DECREF(val);
  return NULL;
}

static Object *array_length(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = gvector_size(arr->vec);
  return integer_new(len);
}

static Object *array_iter(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Object *idx = integer_new(0);
  Object *ret = iter_new(arr->desc, self, idx);
  OB_DECREF(idx);
  return ret;
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
  {"__iter__", "i", "Llang.Iterator<T>;", array_iter},
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

  if (isobj(arr->desc)) {
    RawValue raw;
    gvector_foreach(raw, arr->vec) {
      OB_DECREF(raw.obj);
    }
  }
  gvector_free(arr->vec);
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
  tostr(arr, &sbuf);
  strbuf_append(&sbuf, "]");
  Object *str = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);

  return str;
}

static Object *array_iter_next(Object *iter, Object *step)
{
  if (!iter_check(iter)) {
    error("object of '%.64s' is not an Iterator", OB_TYPE_NAME(iter));
    return NULL;
  }

  Object *ob = iter_obj(iter);
  Object *idx = iter_args(iter);
  ArrayObject *arr = (ArrayObject *)ob;
  int64_t index = integer_asint(idx);
  int64_t by = integer_asint(step);
  debug("array-iter, index: %"PRId64", by: %"PRId64, index, by);

  Object *ret = NULL;
  if (index < gvector_size(arr->vec)) {
    ret = array_getitem(ob, idx);
    integer_setint(idx, index + by);
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
  array_type.desc = desc_from_array;
  type_add_tp(&array_type, "T", NULL);
  if (type_ready(&array_type) < 0)
    panic("Cannot initalize 'Array' type.");
}

Object *array_new(TypeDesc *desc, GVector *vec)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = TYPE_INCREF(desc);
  arr->vec = vec;
  return (Object *)arr;
}

int array_set(Object *self, int index, Object *v)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return -1;
  }

  ArrayObject *arr = (ArrayObject *)self;

  int size = gvector_size(arr->vec);
  if (index < 0 || index > size) {
    error("index %d out of range(0...%d)", index, size);
    return -1;
  }

  RawValue raw = {0};
  gvector_get(get_vec(arr), index, &raw);
  if (isobj(arr->desc)) {
    OB_DECREF(raw.obj);
  }

  unbox(arr->desc, v, &raw);
  gvector_set(get_vec(arr), index, &raw);
  return 0;
}

int array_size(Object *self)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return 0;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return gvector_size(arr->vec);
}

char *array_raw(Object *self)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return gvector_toarr(arr->vec);
}
