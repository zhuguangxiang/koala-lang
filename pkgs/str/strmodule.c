/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without resiction, including without limitation the rights
 to use, copy, modify, merge, publish, disibute, sublicense, and/or sell
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

#include "sbldrobject.h"
#include "arrayobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "resultobject.h"
#include "iomodule.h"
#include "moduleobject.h"

static Object *sbldr_result_ok(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_ok(iob);
  OB_DECREF(iob);
  return res;
}

static Object *sbldr_result_error(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_err(iob);
  OB_DECREF(iob);
  return res;
}

/* func write(buf [byte]) Result<int, Error> */
static Object *sbldr_write(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  char *ptr = array_ptr(args);
  int len = array_len(args);
  int ret = slice_push_array(&sbldr->buf, ptr, len);
  return ret < 0 ? sbldr_result_error(ret) : sbldr_result_ok(len);
}

/* func write_str(s string) Result<int, Error> */
static Object *sbldr_write_str(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  char *ptr = string_asstr(args);
  int len = string_len(args);
  int ret = slice_push_array(&sbldr->buf, ptr, len);
  return ret < 0 ? sbldr_result_error(ret) : sbldr_result_ok(len);
}

static Object *sbldr_length(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  return integer_new(slice_len(&sbldr->buf));
}

static Object *sbldr_as_str(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  char *ptr = slice_ptr(&sbldr->buf, 0);
  int len = slice_len(&sbldr->buf);
  return string_with_len(ptr, len);
}

static Object *sbldr_as_bytes(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  Slice buf;
  slice_copy(&buf, &sbldr->buf);
  return byte_array_with_buf(buf);
}

static Object *sbldr_as_slice(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  Slice buf;
  slice_slice_to_end(&buf, &sbldr->buf, 0);
  return byte_array_with_buf(buf);
}

static Object *sbldr_flush(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }
  return NULL;
}

static Object *sbldr_alloc(TypeObject *type)
{
  SBldrObject *sbldr = kmalloc(sizeof(*sbldr));
  init_object_head(sbldr, &sbldr_type);
  slice_init(&sbldr->buf, 1);
  return (Object *)sbldr;
}

static void sbldr_free(Object *self)
{
  SBldrObject *sbldr = (SBldrObject *)self;
  slice_fini(&sbldr->buf);
  kfree(sbldr);
}

static Object *sbldr_str(Object *self, Object *args)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  char *ptr = slice_ptr(&sbldr->buf, 0);
  int len = slice_len(&sbldr->buf);
  len = (len > 32) ? 32 : len;
  return string_with_len(ptr, len);
}

#define RESULT  "Llang.Result(i)(Llang.Error;);"

static MethodDef sbldr_methods[] = {
  {"write",     "[b", RESULT, sbldr_write     },
  {"write_str", "s",  RESULT, sbldr_write_str },
  {"len",       NULL, "i",  sbldr_length  },
  {"as_str",    NULL, "s",  sbldr_as_str  },
  {"as_bytes",  NULL, "[b", sbldr_as_bytes},
  {"as_slice",  NULL, "[b", sbldr_as_slice},
  {"flush",     NULL, NULL, sbldr_flush},
  {NULL}
};

TypeObject sbldr_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Builder",
  .alloc   = sbldr_alloc,
  .free    = sbldr_free,
  .str     = sbldr_str,
  .methods = sbldr_methods,
};

void init_sbldr_type(void)
{
  sbldr_type.desc = desc_from_klass("str", "Builder");
  type_add_base(&sbldr_type, &io_writer_type);
  if (type_ready(&sbldr_type) < 0) {
    panic("Cannot initalize 'Builder' type.");
  }
}

Slice *sbldr_slice(Object *self)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  return &sbldr->buf;
}

int sbldr_len(Object *self)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return -1;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  return slice_len(&sbldr->buf);
}

char *sbldr_ptr(Object *self)
{
  if (!sbldr_check(self)) {
    error("object of '%.64s' is not a str.Builder", OB_TYPE_NAME(self));
    return NULL;
  }

  SBldrObject *sbldr = (SBldrObject *)self;
  return slice_ptr(&sbldr->buf, 0);
}

void init_str_module(void)
{
  init_sbldr_type();

  Object *m = module_new("str");
  module_add_type(m, &sbldr_type);
  //module_add_funcdefs(m, funcs);
  module_install("str", m);
  OB_DECREF(m);
}

void fini_str_module(void)
{
  type_fini(&sbldr_type);
  module_uninstall("str");
}
