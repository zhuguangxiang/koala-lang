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

typedef int (*elem_cmp_t)(const void *, const void *);

static int elem_int_cmp(const void *v1, const void *v2)
{
  return *(int64_t *)v1 - *(int64_t *)v2;
}

static elem_cmp_t __cmp(TypeDesc *desc)
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

#define slice_check(arr) ((arr)->buf != &(arr)->_buf_)

static inline Vec *__vec(ArrayObject *arr)
{
  ArrayObject *b = arr->buf;
  return (slice_check(arr)) ? b->buf : b;
}

static inline int __offset(ArrayObject *arr)
{
  return slice_check(arr) ? arr->offset : 0;
}

static inline int __len(ArrayObject *arr)
{
  return slice_check(arr) ? arr->len : ((Vec *)arr->buf)->length;
}

static void byte_tostr(ArrayObject *arr, StrBuf *buf)
{
  char *items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_int(buf, items[offset + i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void int_tostr(ArrayObject *arr, StrBuf *buf)
{
  int64_t *items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_int(buf, items[offset + i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void char_tostr(ArrayObject *arr, StrBuf *buf)
{
  int *items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_wchar(buf, items[offset + i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void float_tostr(ArrayObject *arr, StrBuf *buf)
{
  double *items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append_float(buf, items[offset + i]);
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void bool_tostr(ArrayObject *arr, StrBuf *buf)
{
  int *items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append(buf, items[offset + i] ? "true" : "false");
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void string_tostr(ArrayObject *arr, StrBuf *buf)
{
  Object **items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    strbuf_append(buf, "\"");
    strbuf_append(buf, string_asstr(items[offset + i]));
    strbuf_append(buf, "\"");
    if (i < len - 1)
      strbuf_append(buf, ", ");
  }
}

static void obj_tostr(ArrayObject *arr, StrBuf *buf)
{
  Object **items = vec_toarr(__vec(arr));
  int offset = __offset(arr);
  int len = __len(arr);
  for (int i = 0; i < len; ++i) {
    Object *str = object_call(items[offset + i], "__str__", NULL);
    strbuf_append(buf, string_asstr(str));
    OB_DECREF(str);
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

static Object *array_push_back(Object *self, Object *val)
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
  RawValue raw;
  vec_remove(__vec(arr), __offset(arr) + __len(arr) - 1, &raw);
  if (slice_check(arr)) --arr->len;
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
  Object *val = tuple_get(args, 1);
  RawValue raw = obj_to_raw(arr->desc, val);
  vec_insert(__vec(arr), __offset(arr) + integer_asint(index), &raw);
  OB_DECREF(index);
  if (!elem_isobj(arr->desc)) OB_DECREF(val);
  if (slice_check(arr)) ++arr->len;
  return NULL;
}

static Object *array_remove(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  RawValue raw;
  vec_remove(__vec(arr), __offset(arr) + integer_asint(args), &raw);
  if (slice_check(arr)) --arr->len;
  return obj_from_raw(arr->desc, raw);
}

static Object *array_sort(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  Vec *vec = __vec(arr);
  Vec svec = {
    .elems = vec->elems + __offset(arr),
    .objsize = vec->objsize,
    .length = __len(arr),
  };
  vec_sort(&svec, __cmp(arr->desc));
  return NULL;
}

static Object *array_reverse(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  vec_reverse(__vec(arr), __offset(arr), __len(arr));
  return NULL;
}

static Object *array_getitem(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = __len(arr);
  int index = integer_asint(args);
  if (index < 0 || index >= len) {
    error("index %d out of range(0..<%d)", index, len);
    return NULL;
  }

  RawValue raw;
  vec_get(__vec(arr), __offset(arr) + index, &raw);
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

static Object *slice_new(ArrayObject *arr, int offset, int len)
{
  ArrayObject *slice = kmalloc(sizeof(*slice));
  init_object_head(slice, &array_type);
  slice->desc = TYPE_INCREF(arr->desc);
  slice->offset = __offset(arr) + offset;
  slice->len = len;
  slice->buf = slice_check(arr) ? OB_INCREF(arr->buf) : OB_INCREF(arr);
  return (Object *)slice;
}

static Object *array_slice(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;

  Object *start = tuple_get(args, 0);
  Object *end = tuple_get(args, 1);
  Object *step = tuple_get(args, 2);
  int64_t i = integer_asint(start);
  int64_t j = integer_asint(end);
  int64_t k = integer_asint(step);
  OB_DECREF(start);
  OB_DECREF(end);
  OB_DECREF(step);
  return slice_new(arr, i, j - i);
}

/*
static int raw_equal(TypeDesc *desc, RawValue *val, Object *ob)
{
  int res;
  if (desc->kind != TYPE_BASE) {
    Object *bob = object_call(ob, "__equal__", val->obj);
    res = bool_istrue(bob);
    OB_DECREF(bob);
    return res;
  }

  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    res = (val->ival == byte_asint(ob));
    break;
  case BASE_INT:
    if (byte_check(ob)) {
      res = (val->ival == byte_asint(ob));
    } else {
      res = (val->ival == integer_asint(ob));
    }
    break;
  case BASE_CHAR:
    res = (val->cval == char_asch(ob));
    break;
  case BASE_FLOAT:
    res = (val->fval == float_asflt(ob)); //right?
    break;
  case BASE_BOOL:
    res = (val->bval == ((BoolObject *)ob)->value);
  case BASE_STR:
    res = !strcmp(string_asstr(val->obj), string_asstr(ob));
    break;
  default: {
    Object *bob = object_call(ob, "__equal__", val->obj);
    res = bool_istrue(bob);
    OB_DECREF(bob);
    break;
  }
  }
}
*/

static Object *array_index(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = __len(arr);
  if (len <= 0) {
    return NULL;
  }

  Vec *vec = __vec(arr);
  Vec svec = {
    .elems = vec->elems + __offset(arr),
    .objsize = vec->objsize,
    .length = __len(arr),
  };

  RawValue raw = obj_to_raw(arr->desc, args);
  int index = vec_find(&svec, &raw, __cmp(arr->desc));
  if (index >= 0) return integer_new(index);
  return NULL;
}

static Object *array_lastindex(Object *self, Object *args)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int len = __len(arr);
  if (len <= 0) {
    return NULL;
  }

  Vec *vec = __vec(arr);
  Vec svec = {
    .elems = vec->elems + __offset(arr),
    .objsize = vec->objsize,
    .length = __len(arr),
  };

  RawValue raw = obj_to_raw(arr->desc, args);
  int index = vec_find_rev(&svec, &raw, __cmp(arr->desc));
  if (index >= 0) return integer_new(index);
  return NULL;
}

/*
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

  GVector *svec = gvector_new(0, sizeof(RawValue));
  int basekind = arr->desc->kind == TYPE_BASE;
  if (basekind) {
    if (arr->desc->base == BASE_STR ||
        arr->desc->base == BASE_ANY) {
      basekind = 0;
    }
  }
  RawValue raw = {0};
  for (int i = 0; i < gvector_size(arr->vec); i++) {
    gvector_get(arr->vec, i, &raw);
    if (!basekind) {
      OB_INCREF(raw.obj);
    }
    gvector_push_back(svec, &raw);
  }

  for (int i = 0; i < gvector_size(arr2->vec); i++) {
    gvector_get(arr2->vec, i, &raw);
    if (!basekind) {
      OB_INCREF(raw.obj);
    }
    gvector_push_back(svec, &raw);
  }

  return array_new(arr->desc, svec);
}
*/

static Object *array_join(Object *self, Object *args)
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
  Vec *vec = __vec(arr);
  Vec *vec2 = __vec(arr2);
  vec_join(vec, vec2);
  return OB_INCREF(self);
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
  {"__slice__",   "iii",  "Llang.Array<T>;", array_slice},
  {"index",       "<T>",  "i",    array_index},
  {"last_index",   "<T>",  "i",    array_lastindex},
  // {"__add__",     "Llang.Array<T>;", "Llang.Array<T>;", array_concat},
  //{"join",        "Llang.Array<T>;", "Llang.Array<T>;", array_join},
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
  if (!slice_check(arr)) {
    if (elem_isobj(arr->desc)) {
      void **pptr;
      int i;
      vec_foreach(pptr, i, __vec(arr)) {
        OB_DECREF(*pptr);
      }
    }
    vec_fini(arr->buf);
  } else {
    OB_DECREF(arr->buf);
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
  arr->buf = &arr->_buf_;
  vec_init(arr->buf, elem_size(desc));
  return (Object *)arr;
}

Object *byte_array_new(void)
{
  TypeDesc *desc = desc_from_byte;
  Object *bytes = array_new(desc);
  TYPE_DECREF(desc);
  return bytes;
}

int array_set(Object *self, int index, Object *v)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return -1;
  }

  ArrayObject *arr = (ArrayObject *)self;
  int offset = __offset(arr);
  int len = __len(arr);
  if (index < 0 || index > len) {
    error("index %d out of range(0...%d)", index, len);
    return -1;
  }

  RawValue raw;
  int isobj = elem_isobj(arr->desc);
  if (index != len && isobj) {
    vec_get(__vec(arr), offset + index, &raw);
    OB_DECREF(raw.ptr);
  }

  raw = obj_to_raw(arr->desc, v);
  vec_set(__vec(arr), offset + index, &raw);
  if (isobj) OB_INCREF(v);
  if (slice_check(arr) && index == len) ++arr->len;
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

Vec *array_raw(Object *self)
{
  if (!array_check(self)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(self));
    return NULL;
  }

  ArrayObject *arr = (ArrayObject *)self;
  return __vec(arr);
}
