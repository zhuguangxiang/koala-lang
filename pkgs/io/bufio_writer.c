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

#define RESULT  "Llang.Result(i)(Llang.Error;);"
#define DEFAULT_BUF_SIZE  (1024 * 4)

static Object *bufio_result_ok(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_ok(iob);
  OB_DECREF(iob);
  return res;
}

typedef struct bufwriterobject {
  OBJECT_HEAD
  Object *ob;
  Slice buf;
} BufWriterObject;

static Object *bufio_w_alloc(TypeObject *type)
{
  BufWriterObject *b = kmalloc(sizeof(BufWriterObject));
  init_object_head(b, type);
  return (Object *)b;
}

static void bufio_w_free(Object *ob)
{
  BufWriterObject *b = (BufWriterObject *)ob;
  OB_DECREF(b->ob);
  slice_fini(&b->buf);
  kfree(b);
}

static Object *bufio_w_init(Object *self, Object *arg)
{
  if (!bufio_w_check(self)) {
    error("object of '%.64s' is not a BufWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  BufWriterObject *b = (BufWriterObject *)self;
  b->ob = OB_INCREF(arg);
  slice_init_capacity(&b->buf, 1, DEFAULT_BUF_SIZE);
  return NULL;
}

static void flush_buf(BufWriterObject *b)
{
  Slice buf;
  slice_slice_to_end(&buf, &b->buf, 0);
  Object *bytes = byte_array_with_buf(buf);
  Object *ret = object_call(b->ob, "write", bytes);
  OB_DECREF(ret);
  OB_DECREF(bytes);
  slice_clear(&b->buf);
  ret = object_call(b->ob, "flush", NULL);
  OB_DECREF(ret);
}

/*
  func write(buf [byte]) Result<int, Error>;
  func write_str(s string) Result<int, Error> {...}
  func flush();
 */

static Object *bufio_w_write(Object *self, Object *arg)
{
  if (!bufio_w_check(self)) {
    error("object of '%.64s' is not a BufWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  BufWriterObject *b = (BufWriterObject *)self;
  Slice *dst = &b->buf;
  Slice *src = array_slice(arg);

  if (slice_len(dst) + slice_len(src) > slice_capacity(dst)) {
    flush_buf(b);
  }

  slice_extend(dst, src);
  return bufio_result_ok(slice_len(src));
}

static Object *bufio_w_write_str(Object *self, Object *arg)
{
  if (!bufio_w_check(self)) {
    error("object of '%.64s' is not a BufWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  BufWriterObject *b = (BufWriterObject *)self;
  Slice *dst = &b->buf;
  int len = string_len(arg);

  if (slice_len(dst) + len > slice_capacity(dst)) {
    flush_buf(b);
  }

  void *ptr = string_asstr(arg);
  slice_push_array(dst, ptr, len);
  return bufio_result_ok(len);
}

static Object *bufio_w_flush(Object *self, Object *arg)
{
  if (!bufio_w_check(self)) {
    error("object of '%.64s' is not a BufWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  BufWriterObject *b = (BufWriterObject *)self;
  flush_buf(b);
  return NULL;
}

static MethodDef bufio_w_methods[] = {
  {"__init__", "Lio.Writer;", NULL, bufio_w_init},
  {"write", "[b", RESULT, bufio_w_write},
  {"write_str", "s", RESULT, bufio_w_write_str},
  {"flush", NULL, NULL, bufio_w_flush},
  {NULL}
};

TypeObject bufio_w_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "BufWriter",
  .flags = TPFLAGS_CLASS,
  .alloc = bufio_w_alloc,
  .free  = bufio_w_free,
  .methods = bufio_w_methods,
};

/*-------------------------------------------------------------------------*/

typedef struct linewriterobject {
  OBJECT_HEAD
  Object *ob;
} LineWriterObject;

static Object *line_alloc(TypeObject *type)
{
  LineWriterObject *line = kmalloc(sizeof(LineWriterObject));
  init_object_head(line, type);
  return (Object *)line;
}

static void line_free(Object *ob)
{
  LineWriterObject *line = (LineWriterObject *)ob;
  OB_DECREF(line->ob);
  kfree(line);
}

static Object *line_init(Object *self, Object *arg)
{
  if (!line_w_check(self)) {
    error("object of '%.64s' is not a LineWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  LineWriterObject *line = (LineWriterObject *)self;
  Object *buf = bufio_w_alloc(&bufio_w_type);
  bufio_w_init(buf, arg);
  line->ob = buf;
  return NULL;
}

static inline void flush_line(LineWriterObject *line)
{
  bufio_w_flush(line->ob, NULL);
}

/*
  func write(buf [byte]) Result<int, Error>;
  func write_str(s string) Result<int, Error> {...}
  func flush();
 */

static Object *line_write(Object *self, Object *arg)
{
  if (!line_w_check(self)) {
    error("object of '%.64s' is not a LineWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  LineWriterObject *line = (LineWriterObject *)self;
  char *ptr = array_ptr(arg);
  int len = array_len(arg);
  int idx = mem_nrchr(ptr, len, '\n');
  if (idx < 0) return bufio_w_write(line->ob, arg);

  Object *bytes;
  Object *ret;
  Slice buf;

  // write partial and flush
  slice_slice(&buf, array_slice(arg), 0, idx + 1);
  bytes = byte_array_with_buf(buf);
  ret = bufio_w_write(line->ob, bytes);
  OB_DECREF(bytes);
  if (!result_test(ret)) return ret;
  OB_DECREF(ret);
  flush_line(line);

  // write left
  slice_slice_to_end(&buf, array_slice(arg), idx + 1);
  bytes = byte_array_with_buf(buf);
  ret = bufio_w_write(line->ob, bytes);
  OB_DECREF(bytes);
  if (!result_test(ret)) return ret;
  OB_DECREF(ret);
  return bufio_result_ok(len);
}

static Object *line_write_str(Object *self, Object *arg)
{
  Slice buf;
  int len = string_len(arg);
  slice_init_capacity(&buf, 1, len + 1);
  slice_push_array(&buf, string_asstr(arg), len);
  Object *bytes = byte_array_with_buf(buf);
  Object *ret = line_write(self, bytes);
  OB_DECREF(bytes);
  return ret;
}

static Object *line_flush(Object *self, Object *arg)
{
  if (!line_w_check(self)) {
    error("object of '%.64s' is not a LineWriter", OB_TYPE_NAME(self));
    return NULL;
  }

  LineWriterObject *line = (LineWriterObject *)self;
  flush_line(line);
  return NULL;
}

static MethodDef line_w_methods[] = {
  {"__init__", "Lio.Writer;", NULL, line_init},
  {"write", "[b", RESULT, line_write},
  {"write_str", "s", RESULT, line_write_str},
  {"flush", NULL, NULL, line_flush},
  {NULL}
};

TypeObject line_w_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "LineWriter",
  .flags = TPFLAGS_CLASS,
  .alloc = line_alloc,
  .free  = line_free,
  .methods = line_w_methods,
};
