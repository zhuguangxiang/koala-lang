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
#include "sbldrobject.h"
#include "utf8.h"

#define RESULT  "Llang.Result(i)(Llang.Error;);"

static Object *io_result_error(int val)
{
  Object *iob = integer_new(val);
  Object *res = result_err(iob);
  OB_DECREF(iob);
  return res;
}

/*
  func read(buf [byte], nbytes int) Result<int, Error>;

  func read_to_end(buf [byte]) Result<int, Error> {
    ...
  }

  func read_to_str(sb str.Buffer) Result<int, Error> {
    ...
  }
 */

static Object *read_to_end(Object *self, Object *args)
{
  int len = 0;
  int n;
  Object *res;
  Object *val;
  Object *read_args = tuple_encode("Oi", args, 32);

  while (1) {
    res = object_call(self, "read", read_args);
    if (!result_test(res))
      break;

    val = result_get_ok(res, NULL);
    OB_DECREF(res);
    n = integer_asint(val);
    OB_DECREF(val);
    if (n <= 0) {
      val = integer_new(len);
      res = result_ok(val);
      OB_DECREF(val);
      break;
    } else {
      len += n;
    }
  }

  OB_DECREF(read_args);
  return res;
}

/* func read_to_str(sb str.Buffer) Result<int, Error> */
static Object *read_to_str(Object *self, Object *args)
{
  Object *bytes = byte_array_no_buf();
  Slice *slice = array_slice(bytes);
  Slice *buf = sbldr_slice(args);
  slice_slice_to_end(slice, buf, 0);
  Object *res = read_to_end(self, bytes);

  // sava to str.builder
  slice_fini(buf);
  slice_slice_to_end(buf, slice, 0);
  OB_DECREF(bytes);

  if (!result_test(res))
    return res;

  char *ptr = sbldr_ptr(args);
  int len = sbldr_len(args);
  if (check_utf8_valid_with_len(ptr, len) < 0) {
    OB_DECREF(res);
    error("not valid utf8 string.");
    return io_result_error(-1);
  }
  return res;
}

static MethodDef reader_methods[]= {
  {"read", "[bi", RESULT, NULL, 1},
  {"read_to_end", "[b", RESULT, read_to_end},
  {"read_to_str", "Lstr.Builder;", RESULT, read_to_str},
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

  func flush();
 */
static Object *write_str(Object *self, Object *args)
{
  Object *bytes = byte_array_new();
  Slice *slice = array_slice(bytes);
  char *str = string_asstr(args);
  int len = string_len(args);
  slice_push_array(slice, str, len);
  Object *res = object_call(self, "write", bytes);
  OB_DECREF(bytes);
  return res;
}

static MethodDef writer_methods[]= {
  {"write", "[b", RESULT, NULL, 1},
  {"write_str", "s", RESULT, write_str},
  {"flush", NULL, NULL, NULL, 1},
  {NULL}
};

TypeObject io_writer_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Writer",
  .flags = TPFLAGS_TRAIT,
  .methods = writer_methods,
};

static void init_bufio(void)
{
  bufio_r_type.desc = desc_from_klass("io", "BufReader");
  type_add_base(&bufio_r_type, &io_reader_type);
  if (type_ready(&bufio_r_type) < 0)
    panic("Cannot initalize 'BufWriter' type.");

  bufio_w_type.desc = desc_from_klass("io", "BufWriter");
  type_add_base(&bufio_w_type, &io_writer_type);
  if (type_ready(&bufio_w_type) < 0)
    panic("Cannot initalize 'BufWriter' type.");

  line_w_type.desc = desc_from_klass("io", "LineWriter");
  type_add_base(&line_w_type, &io_writer_type);
  if (type_ready(&line_w_type) < 0)
    panic("Cannot initalize 'LineWriter' type.");
}

static void fini_bufio(void)
{
  type_fini(&bufio_r_type);
  type_fini(&bufio_w_type);
  type_fini(&line_w_type);
}

void init_io_types(void)
{
  io_writer_type.desc = desc_from_klass("io", "Writer");
  if (type_ready(&io_writer_type) < 0)
    panic("Cannot initalize 'Writer' type.");

  io_reader_type.desc = desc_from_klass("io", "Reader");
  if (type_ready(&io_reader_type) < 0)
    panic("Cannot initalize 'Reader' type.");

  init_bufio();
}

void fini_io_types(void)
{
  type_fini(&io_writer_type);
  type_fini(&io_reader_type);
  fini_bufio();
}

void init_io_module(void)
{
  Object *m = module_new("io");
  module_add_type(m, &io_writer_type);
  module_add_type(m, &io_reader_type);
  module_add_type(m, &bufio_r_type);
  module_add_type(m, &bufio_w_type);
  module_add_type(m, &line_w_type);
  module_install("io", m);
  OB_DECREF(m);
}

void fini_io_module(void)
{
  module_uninstall("io");
}
