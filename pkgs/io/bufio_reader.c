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

#include "iomodule.h"
#include "slice.h"
#include "arrayobject.h"
#include "stringobject.h"
#include "resultobject.h"
#include "intobject.h"
#include "tupleobject.h"
#include "sbldrobject.h"

#define RESULT  "Llang.Result(i)(Llang.Error;);"
#define DEFAULT_BUF_SIZE  (1024 * 4)

static Object *bufio_result_ok(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_ok(iob);
  OB_DECREF(iob);
  return res;
}

static int64_t bufio_result_get_ok(Object *res)
{
  Object *iob = result_get_ok(res, NULL);
  int64_t val = integer_asint(iob);
  OB_DECREF(iob);
  return val;
}

typedef struct bufreaderobject {
  OBJECT_HEAD
  Object *ob;
  int pos;
  int cap;
  Object *buf;
} BufReaderObject;

static Object *bufio_r_alloc(TypeObject *type)
{
  BufReaderObject *b = kmalloc(sizeof(BufReaderObject));
  init_object_head(b, type);
  return (Object *)b;
}

static void bufio_r_free(Object *ob)
{
  BufReaderObject *b = (BufReaderObject *)ob;
  OB_DECREF(b->ob);
  OB_DECREF(b->buf);
  kfree(b);
}

static Object *bufio_r_init(Object *self, Object *arg)
{
  if (!bufio_r_check(self)) {
    error("object of '%.64s' is not a BufReader", OB_TYPE_NAME(self));
    return NULL;
  }

  BufReaderObject *b = (BufReaderObject *)self;
  b->ob = OB_INCREF(arg);
  b->pos = 0;
  b->cap = 0;
  b->buf = byte_array_no_buf();
  slice_init_capacity(array_slice(b->buf), 1, DEFAULT_BUF_SIZE);
  return NULL;
}

static Object *fill_buf(BufReaderObject *b)
{
  printf("read\n");
  Slice *buf = array_slice(b->buf);
  slice_clear(buf);
  Object *read_arg = tuple_encode("Oi", b->buf, DEFAULT_BUF_SIZE);
  Object *ret = object_call(b->ob, "read", read_arg);
  OB_DECREF(read_arg);
  if (result_test(ret)) {
    b->pos = 0;
    b->cap = (int)bufio_result_get_ok(ret);
  }
  return ret;
}

/* func read(buf [byte], nbytes int) Result<int, Error>; */
static Object *bufio_r_read(Object *self, Object *arg)
{
  if (!bufio_r_check(self)) {
    error("object of '%.64s' is not a BufReader", OB_TYPE_NAME(self));
    return NULL;
  }

  BufReaderObject *b = (BufReaderObject *)self;

  if (b->pos >= b->cap) {
    expect(b->pos == b->cap);
    Object *ret = fill_buf(b);
    if (!result_test(ret)) return ret;
    OB_DECREF(ret);
  }

  Object *arr = NULL;
  int64_t nbytes = 0;
  tuple_decode(arg, "Oi", &arr, &nbytes);
  Slice *buf = array_slice(arr);
  int start_len = slice_len(buf);
  slice_reserve(buf, nbytes);
  void *dst = slice_ptr(buf, start_len);
  void *src = slice_ptr(array_slice(b->buf), b->pos);
  int count = MIN(b->cap - b->pos, nbytes);
  memcpy(dst, src, count);
  b->pos += count;
  OB_DECREF(arr);
  return bufio_result_ok(count);
}

static int __slice_byte_cmp__(const void *v1, const void *v2)
{
  return *(const char *)v1 - *(const char *)v2;
}

/* func read_until(buf [byte], delim byte) Result<int, Error>; */
static Object *bufio_r_read_until(Object *self, Object *arg)
{
  if (!bufio_r_check(self)) {
    error("object of '%.64s' is not a BufReader", OB_TYPE_NAME(self));
    return NULL;
  }

  BufReaderObject *b = (BufReaderObject *)self;
  int done = 0;
  int read = 0;
  while (1) {
    if (b->pos >= b->cap) {
      expect(b->pos == b->cap);
      Object *ret = fill_buf(b);
      if (!result_test(ret)) return ret;
      OB_DECREF(ret);
    }

    Object *arr;
    int delim;
    tuple_decode(arg, "Ob", &arr, &delim);
    Slice *buf = array_slice(arr);
    Slice nxt;
    slice_slice(&nxt, array_slice(b->buf), b->pos, b->cap - b->pos);
    int idx = slice_index(&nxt, &delim, __slice_byte_cmp__);
    Slice from;
    int used = 0;
    if (idx >= 0) {
      used = idx + 1;
      done = 1;
      slice_slice(&from, &nxt, 0, idx);
    } else {
      slice_slice_to_end(&from, &nxt, 0);
      used = slice_len(&nxt);
    }
    slice_extend(buf, &from);
    slice_fini(&from);
    slice_fini(&nxt);
    OB_DECREF(arr);
    b->pos += used;
    read += used;
    if (done || used == 0)
      break;
  }
  return bufio_result_ok(read);
}

/* func read_line(sb str.Builder) Result<int, Error>; */
static Object *bufio_r_read_line(Object *self, Object *arg)
{
  if (!bufio_r_check(self)) {
    error("object of '%.64s' is not a BufReader", OB_TYPE_NAME(self));
    return NULL;
  }

  BufReaderObject *b = (BufReaderObject *)self;
  SBldrObject *bldr = (SBldrObject *)arg;
  Object *read_arg = tuple_encode("Ob", bldr->buf, '\n');
  Object *ret = bufio_r_read_until(self, read_arg);
  OB_DECREF(read_arg);
  return ret;
}

static MethodDef bufio_r_methods[] = {
  {"__init__", "Lio.Reader;", NULL, bufio_r_init},
  {"read", "[bi", RESULT, bufio_r_read},
  {"read_until", "[bb", RESULT, bufio_r_read_until},
  {"read_line", "Lstr.Builder;", RESULT, bufio_r_read_line},
  {"lines"},
  {"split"},
  {NULL}
};

TypeObject bufio_r_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "BufReader",
  .flags = TPFLAGS_CLASS,
  .alloc = bufio_r_alloc,
  .free  = bufio_r_free,
  .methods = bufio_r_methods,
};
