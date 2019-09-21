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

#include "rangeobject.h"
#include "iterobject.h"

static Object *range_length(Object *self, Object *args)
{
  if (!range_check(self)) {
    error("object of '%.64s' is not a Range", OB_TYPE_NAME(self));
    return NULL;
  }

  RangeObject *range = (RangeObject *)self;
  return integer_new(range->len);
}

static Object *range_getitem(Object *self, Object *args)
{
  if (!range_check(self)) {
    error("object of '%.64s' is not a Range", OB_TYPE_NAME(self));
    return NULL;
  }

  RangeObject *range = (RangeObject *)self;
  int index = integer_asint(args);
  if (index < 0 || index >= range->len) {
    error("index %d out of range(0..<%d)", index, range->len);
    return NULL;
  }

  int64_t val = integer_asint(range->start);
  return integer_new(val + index);
}

static Object *range_iter(Object *self, Object *args)
{
  if (!range_check(self)) {
    error("object of '%.64s' is not a Range", OB_TYPE_NAME(self));
    return NULL;
  }

  RangeObject *range = (RangeObject *)self;
  Object *idx = integer_new(0);
  TypeDesc *desc = desc_from_int;
  Object *ret = iter_new(desc, self, idx, args);
  TYPE_DECREF(desc);
  OB_DECREF(idx);
  return ret;
}

static MethodDef range_methods[] = {
  {"length", NULL, "i", range_length},
  {"__getitem__", "i", "i", range_getitem},
  {"__iter__", "i", "Llang.Iterator(i);", range_iter},
  {NULL}
};

static void range_free(Object *ob)
{
  if (!range_check(ob)) {
    error("object of '%.64s' is not a Range", OB_TYPE_NAME(ob));
    return;
  }

  RangeObject *range = (RangeObject *)ob;
  OB_DECREF(range->start);
  kfree(range);
}

static Object *range_iter_next(Object *iter, Object *args)
{
  if (!iter_check(iter)) {
    error("object of '%.64s' is not an Iterator", OB_TYPE_NAME(iter));
    return NULL;
  }

  Object *ob = iter_obj(iter);
  Object *idx = iter_args(iter);
  Object *step = iter_step(iter);
  RangeObject *range = (RangeObject *)ob;
  int64_t index = integer_asint(idx);
  int64_t by = integer_asint(step);
  debug("range-iter, index: %ld, by: %ld", index, by);

  Object *ret = NULL;
  if (index >= 0 && index < range->len) {
    int64_t val = integer_asint(range->start);
    ret = integer_new(val + index);
    integer_setint(idx, index + by);
  }

  return ret;
}

TypeObject range_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Range",
  .free    = range_free,
  .iter    = range_iter,
  .iternext = range_iter_next,
  .methods  = range_methods,
};

void init_range_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Range");
  range_type.desc = desc;
  if (type_ready(&range_type) < 0)
    panic("Cannot initalize 'Range' type.");
}

Object *range_new(Object *start, int len)
{
  RangeObject *range = kmalloc(sizeof(*range));
  init_object_head(range, &range_type);
  range->start = OB_INCREF(start);
  range->len = len;
  return (Object *)range;
}
