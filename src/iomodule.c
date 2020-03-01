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

#include "koala.h"

/*
  func read(buf [byte], count int) Result<int, Error>;

  func read_str(w io.Writer) Result<int, Error> {
    buf := new Array<byte>()
    read(buf, 32)
    return w.write(buf)
  }
 */
static Object *read_str(Object *self, Object *args)
{
  Object *arr = byte_array_new();
  Object *count = integer_new(32);
  Object *tuple = tuple_new(2);
  tuple_set(tuple, 0, arr);
  tuple_set(tuple, 1, count);
  OB_DECREF(arr);
  OB_DECREF(count);
  Object *res = object_call(self, "read", tuple);
  if (result_test(res)) {
    OB_DECREF(res);
    res = object_call(args, "write", arr);
  }
  OB_DECREF(tuple);
  return res;
}

static MethodDef reader_methods[]= {
  {"read", "[bi", "Llang.Result(i)(Llang.Error;);", NULL, 1},
  {"read_str", "Lio.Writer;", "Llang.Result(i)(Llang.Error;);", read_str},
  {NULL}
};

TypeObject io_reader_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Reader",
  .flags = TPFLAGS_TRAIT,
  .methods = reader_methods,
};

/*
  func write(buf [byte]) Result<int, Error>;

  func write_str(s string) Result<int, Error> {
    return write(s.as_bytes())
  }
 */
static Object *write_str(Object *self, Object *args)
{
  Object *arr = string_asbytes(args, NULL);
  Object *res = object_call(self, "write", arr);
  OB_DECREF(arr);
  return res;
}

static MethodDef writer_methods[]= {
  {"write", "[b", "Llang.Result(i)(Llang.Error;);", NULL, 1},
  {"write_str", "s", "Llang.Result(i)(Llang.Error;);", write_str},
  {"flush", NULL, NULL, NULL, 1},
  {NULL}
};

TypeObject io_writer_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Writer",
  .flags = TPFLAGS_TRAIT,
  .methods = writer_methods,
};

void init_io_types(void)
{
  io_writer_type.desc = desc_from_klass("io", "Writer");
  if (type_ready(&io_writer_type) < 0)
    panic("Cannot initalize 'Writer' type.");

  io_reader_type.desc = desc_from_klass("io", "Reader");
  if (type_ready(&io_reader_type) < 0)
    panic("Cannot initalize 'Reader' type.");
}

void fini_io_types(void)
{
  type_fini(&io_writer_type);
  type_fini(&io_reader_type);
}

static Object *_io_print_(Object *self, Object *args)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  IoPrint(args);
  return NULL;
}

static Object *_io_println_(Object *self, Object *args)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  IoPrintln(args);
  return NULL;
}

static MethodDef io_methods[] = {
  {"print",   "A", NULL, _io_print_   },
  {"println", "A", NULL, _io_println_ },
  {NULL}
};

void init_io_module(void)
{
  Object *m = module_new("io");
  module_add_funcdefs(m, io_methods);
  module_add_type(m, &io_writer_type);
  module_add_type(m, &io_reader_type);
  module_not_ready(m);
  module_install("io", m);
  OB_DECREF(m);
}

void fini_io_module(void)
{
  module_uninstall("io");
}

void IoPrint(Object *ob)
{
  if (ob == NULL)
    return;
  if (string_check(ob)) {
    printf("%s", string_asstr(ob));
  } else if (integer_check(ob)) {
    printf("%ld", integer_asint(ob));
  } else if (byte_check(ob)) {
    printf("%d", byte_asint(ob));
  } else if (float_check(ob)) {
    printf("%lf", float_asflt(ob));
  } else {
    Object *str = object_call(ob, "__str__", NULL);
    if (str != NULL) {
      printf("%s", string_asstr(str));
      OB_DECREF(str);
    } else {
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
    }
  }
  fflush(stdout);
}

void IoPrintln(Object *ob)
{
  if (ob == NULL) {
    printf("nil\n");
    return;
  }
  if (string_check(ob)) {
    printf("%s\n", string_asstr(ob));
  } else if (integer_check(ob)) {
    printf("%ld\n", integer_asint(ob));
  } else if (byte_check(ob)) {
    printf("%d\n", byte_asint(ob));
  } else if (float_check(ob)) {
    printf("%lf\n", float_asflt(ob));
  } else {
    Object *str = object_call(ob, "__str__", NULL);
    if (str != NULL) {
      printf("%s\n", string_asstr(str));
      OB_DECREF(str);
    } else {
      error("object of '%.64s' is not printable", OB_TYPE_NAME(ob));
    }
  }
  fflush(stdout);
}
