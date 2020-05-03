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
#include "methodobject.h"
#include "strbuf.h"

static int elem_isobj(TypeDesc *desc)
{
  if (desc->kind != TYPE_BASE)
    return 1;

  char kind = desc->base;
  switch (kind) {
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

static int elem_size(TypeDesc *desc)
{
  if (desc->kind != TYPE_BASE)
    return PTR_SIZE;

  char kind = desc->base;
  switch (kind) {
  case BASE_BYTE:
    return BYTE_SIZE;
  case BASE_INT:
    return INT_SIZE;
  case BASE_CHAR:
    return CHAR_SIZE;
  case BASE_FLOAT:
    return FLT_SIZE;
  case BASE_BOOL:
    return BOOL_SIZE;
  default:
    return PTR_SIZE;
  }
}

static int elem_int_cmp(const void *v1, const void *v2)
{
  return *(int64_t *)v1 - *(int64_t *)v2;
}

static __compar_fn_t __cmp(TypeDesc *desc)
{
  if (desc->kind != TYPE_BASE)
    return NULL;

  char kind = desc->base;
  switch (kind) {
  case BASE_BYTE:
    return NULL;
  case BASE_INT:
    return elem_int_cmp;
  case BASE_CHAR:
    return NULL;
  case BASE_FLOAT:
    return NULL;
  case BASE_BOOL:
    return NULL;
  default:
    return NULL;
  }
}

static inline void *__ptr(ArrayObject *arr)
{
  return slice_ptr(&arr->buf, 0);
}

static inline int __len(ArrayObject *arr)
{
  return slice_len(&arr->buf);
}

static void byte_tostr(ArrayObject *arr, StrBuf *buf)
{
  char *items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_int(buf, items[i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void int_tostr(ArrayObject *arr, StrBuf *buf)
{
  int64_t *items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_int(buf, items[i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void char_tostr(ArrayObject *arr, StrBuf *buf)
{
  int *items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_wchar(buf, items[i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void float_tostr(ArrayObject *arr, StrBuf *buf)
{
  double *items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_float(buf, items[i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void bool_tostr(ArrayObject *arr, StrBuf *buf)
{
  int *items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append(buf, items[i] ? "true" : "false");
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void string_tostr(ArrayObject *arr, StrBuf *buf)
{
  Object **items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append(buf, "\"");
    strbuf_append(buf, string_asstr(items[i]));
    strbuf_append(buf, "\"");
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void obj_tostr(ArrayObject *arr, StrBuf *buf)
{
  Object **items = __ptr(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    if (items[i] == NULL) {
      strbuf_append(buf, "nil");
    } else {
      Object *str = object_call(items[i], "__str__", NULL);
      strbuf_append(buf, string_asstr(str));
      OB_DECREF(str);
    }
    if (i < len - 1)
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

Object *array_push_back(Object *self, Object *val)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  array_set(self, __len(arr), val);
  return NULL;
}

static Object *array_pop_back(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  if (__len(arr) <= 0) {
    error("array is empty.");
    return NULL;
  }

  RawValue raw = {0};
  slice_pop_back(&arr->buf, &raw);
  return obj_from_raw(arr->desc, raw);
}

static Object *array_insert(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Object *index = tuple_get(args, 0);
  int idx = integer_asint(index);
  int len = __len(arr);
  if (idx < 0 || idx > __len(arr)) {
    error("index %d out of range(0...%d)", idx, len);
    OB_DECREF(index);
    return NULL;
  }

  Object *val = tuple_get(args, 1);
  RawValue raw = obj_to_raw(arr->desc, val);
  slice_insert(&arr->buf, idx, &raw);
  if (!elem_isobj(arr->desc)) OB_DECREF(val);
  OB_DECREF(index);
  return NULL;
}

static Object *array_remove(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int idx = integer_asint(args);
  int len = __len(arr);
  if (idx < 0 || idx >= __len(arr)) {
    error("index %d out of range(0..<%d)", idx, __len(arr));
    return NULL;
  }

  RawValue raw = {0};
  slice_remove(&arr->buf, idx, &raw);
  return obj_from_raw(arr->desc, raw);
}

static Object *array_sort(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  slice_sort(&arr->buf, __cmp(arr->desc));
  return NULL;
}

static Object *array_reverse(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  slice_reverse(&arr->buf);
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
  int len = __len(arr);
  if (index < 0 || index >= len) {
    error("index %d out of range(0..<%d)", index, len);
    return NULL;
  }

  RawValue raw = {0};
  slice_get(&arr->buf, index, &raw);
  Object *ob = obj_from_raw(arr->desc, raw);
  if (elem_isobj(arr->desc)) OB_INCREF(ob);
  return ob;
}

static Object *array_setitem(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Object *index = tuple_get(args, 0);
  int idx = integer_asint(index);
  int len = __len(arr);
  if (idx < 0 || idx >= len) {
    OB_DECREF(index);
    error("index %d out of range(0..<%d)", idx, len);
    return NULL;
  }

  Object *val = tuple_get(args, 1);
  array_set(self, idx, val);
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
  return integer_new(__len(arr));
}

/*
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
*/

static Object *slice_new(ArrayObject *arr, int offset, int length)
{
  ArrayObject *slice = kmalloc(sizeof(*slice));
  init_object_head(slice, &array_type);
  slice->desc = TYPE_INCREF(arr->desc);
  slice_slice(&slice->buf, &arr->buf, offset, length);
  return (Object *)slice;
}

static Object *array_as_slice(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;

  Object *start = tuple_get(args, 0);
  Object *end = tuple_get(args, 1);
  int64_t i = integer_asint(start);
  int64_t j = integer_asint(end);
  OB_DECREF(start);
  OB_DECREF(end);

  int len = __len(arr);
  if (i < 0) i = 0;
  if (j < 0) j = len;

  if (j < i) {
    error("start index %ld must be less than end index %ld", i, j);
    return NULL;
  }

  if (j > len) {
    error("range(%ld..<%ld) is out of range(0..<%d)", i, j, len);
    return NULL;
  }

  return slice_new(arr, i, j - i);
}

static Object *array_index(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = __len(arr);
  if (len <= 0) {
    error("array is empty.");
    return NULL;
  }

  RawValue raw = obj_to_raw(arr->desc, args);
  int index = slice_index(&arr->buf, &raw, __cmp(arr->desc));
  if (index >= 0) return integer_new(index);
  return NULL;
}

static Object *array_last_index(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = __len(arr);
  if (len <= 0) {
    error("array is empty.");
    return NULL;
  }

  RawValue raw = obj_to_raw(arr->desc, args);
  int index = slice_last_index(&arr->buf, &raw, __cmp(arr->desc));
  if (index >= 0) return integer_new(index);
  return NULL;
}

static Object *array_concat(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!array_check(args)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(args));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  ArrayObject *arr2 = (ArrayObject *)args;
  ArrayObject *newarr = (ArrayObject *)array_new(arr->desc);
  slice_extend(&newarr->buf, &arr->buf);
  slice_extend(&newarr->buf, &arr2->buf);
  if (elem_isobj(arr->desc)) {
    Object **ptr;
    int i;
    slice_foreach(ptr, i, &newarr->buf) {
      OB_INCREF(*ptr);
    }
  }
  return (Object *)newarr;
}

static Object *array_extend(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!array_check(args)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(args));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  ArrayObject *arr2 = (ArrayObject *)args;
  slice_extend(&arr->buf, &arr2->buf);
  if (elem_isobj(arr2->desc)) {
    Object **ptr;
    int i;
    slice_foreach(ptr, i, &arr2->buf) {
      OB_INCREF(*ptr);
    }
  }
  return OB_INCREF(self);
}

static Object *array_reserve(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int64_t size = integer_asint(args);
  slice_expand(&arr->buf, size);
  return NULL;
}

static Object *array_swap(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Object *x = tuple_get(args, 0);
  Object *y = tuple_get(args, 1);
  int64_t i = integer_asint(x);
  int64_t j = integer_asint(y);
  OB_DECREF(x);
  OB_DECREF(y);
  int len = __len(arr);
  if (i < 0 || i > len) {
    error("index %ld out of range(0..<%d)", i, len);
    return NULL;
  }
  if (j < 0 || j > len) {
    error("index %ld out of range(0..<%d)", j, len);
    return NULL;
  }

  slice_swap(&arr->buf, i, j);
  return NULL;
}

static Object *array_clear(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  slice_clear(&arr->buf);
  return NULL;
}

static Object *array_empty(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int empty = slice_empty(&arr->buf);
  return empty ? bool_true() : bool_false();
}

static MethodDef array_methods[] = {
  {"push_back",   "<T>",  NULL,   array_push_back },
  {"pop_back",    NULL,   "<T>",  array_pop_back  },
  {"insert",      "i<T>", NULL,   array_insert  },
  {"remove",      "i",    "<T>",  array_remove  },
  {"sort",        NULL,   NULL,   array_sort    },
  {"reverse",     NULL,   NULL,   array_reverse },
  {"len",         NULL,   "i",    array_length  },
  {"__getitem__", "i",    "<T>",  array_getitem },
  {"__setitem__", "i<T>", NULL,   array_setitem },
  /*
  {"__iter__", "i", "Llang.Iterator<T>;", array_iter},
  */
  {"__slice__",   "iii",  "Llang.Array<T>;", array_as_slice},
  {"index",       "<T>",  "i",    array_index},
  {"last_index",  "<T>",  "i",    array_last_index},
  {"__add__",     "Llang.Array<T>;", "Llang.Array<T>;", array_concat},
  {"extend",      "Llang.Array<T>;", "Llang.Array<T>;", array_extend},
  {"reserve",     "i",    NULL, array_reserve},
  {"swap",        "Llang.Array<T>;ii",  NULL, array_swap},
  {"clear",       "Llang.Array<T>;", NULL, array_clear},
  {"is_empty",    "Llang.Array<T>;", "z",  array_empty},
  {"sort_by",     "P<T><T>:i", NULL,  NULL},
  {"bsearch", "<T>", "i", NULL},
  {"binary_search_by", "<T>", "i", NULL},
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
  if (elem_isobj(arr->desc)) {
    Object **ptr;
    int i;
    slice_foreach(ptr, i, &arr->buf) {
      OB_DECREF(*ptr);
    }
  }
  slice_fini(&arr->buf);
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

/*
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
*/

TypeObject array_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Array",
  .free    = array_free,
  .str     = array_str,
  .iter    = NULL,
  .iternext = NULL,
  .methods  = array_methods,
};

void init_array_type(void)
{
  array_type.desc = desc_from_array;
  type_add_tp(&array_type, "T", NULL);
  if (type_ready(&array_type) < 0)
    panic("Cannot initalize 'Array' type.");
}

Object *array_new(TypeDesc *desc)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = TYPE_INCREF(desc);
  slice_init(&arr->buf, elem_size(desc));
  return (Object *)arr;
}

Object *array_with_buf(TypeDesc *desc, Slice buf)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = TYPE_INCREF(desc);
  arr->buf = buf;
  return (Object *)arr;
}

Object *byte_array_new(void)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = desc_from_byte;
  slice_init(&arr->buf, elem_size(arr->desc));
  return (Object *)arr;
}

Object *byte_array_with_buf(Slice buf)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = desc_from_byte;
  arr->buf = buf;
  return (Object *)arr;
}

Object *byte_array_no_buf(void)
{
  ArrayObject *arr = kmalloc(sizeof(*arr));
  init_object_head(arr, &array_type);
  arr->desc = desc_from_byte;
  return (Object *)arr;
}

int array_set(Object *self, int index, Object *v)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return -1;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = __len(arr);
  if (index < 0 || index > len) {
    error("index %d out of range(0...%d)", index, len);
    return -1;
  }

  RawValue raw = {0};
  int isobj = elem_isobj(arr->desc);
  if (index != len && isobj) {
    slice_get(&arr->buf, index, &raw);
    OB_DECREF(raw.ptr);
  }

  raw = obj_to_raw(arr->desc, v);
  slice_set(&arr->buf, index, &raw);
  if (isobj) OB_INCREF(v);
  return 0;
}

int array_len(Object *self)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return 0;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return __len(arr);
}

Slice *array_slice(Object *self)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return &arr->buf;
}

void *array_ptr(Object *self)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return slice_ptr(&arr->buf, 0);
}
