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

#include "sbufobject.h"
#include "moduleobject.h"

static Object *sbuf_write(Object *self, Object *args)
{

}

static Object *sbuf_write_str(Object *self, Object *args)
{

}

static Object *sbuf_write_fmt(Object *self, Object *args)
{

}

static Object *sbuf_length(Object *self, Object *args)
{

}

static Object *sbuf_as_str(Object *self, Object *args)
{

}

static Object *sbuf_as_bytes(Object *self, Object *args)
{
  if (!sbuf_check(self)) {
    error("object of '%.64s' is not a StringBuffer", OB_TYPE_NAME(self));
    return NULL;
  }

  SBufObject *sbuf = (SBufObject *)self;
  return OB_INCREF(sbuf->buf);
}

static Object *sbuf_alloc(TypeObject *type)
{
  SBufObject *sbuf = kmalloc(sizeof(*sbuf));
  init_object_head(sbuf, &sbuf_type);
  sbuf->buf = byte_array_new();
  return (Object *)sbuf;
}

static void sbuf_free(Object *self)
{

}

static MethodDef sbuf_methods[] = {
  {"write",     "[b", "Llang.Result(i)(Llang.Error;);", sbuf_write     },
  {"write_str", "s",  "Llang.Result(i)(Llang.Error;);", sbuf_write_str },
  {"length",    NULL, "i",  sbuf_length  },
  {"as_str",    NULL, "s",  sbuf_as_str  },
  {"as_bytes",  NULL, "[b", sbuf_as_bytes},
  {NULL}
};

TypeObject sbuf_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "StringBuffer",
  .alloc   = sbuf_alloc,
  .free    = sbuf_free,
  .methods = sbuf_methods,
};

void init_sbuf_type(void)
{
  sbuf_type.desc = desc_from_klass("str", "StringBuffer");
  if (type_ready(&sbuf_type) < 0) {
    panic("Cannot initalize 'StringBuffer' type.");
  }
}
