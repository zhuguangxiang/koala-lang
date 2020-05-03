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
  func read(buf [byte]) Result<int, Error>;

  func read_to_end(buf [byte]) Result<int, Error> {
    start_len := buf.len()
    len := buf.len()
    var ret Result<int, Error>
    while {
      if len == buf.len() {
        buf.reserve(32)
      }
      match read(buf[len :]) {
        Ok(0) => {
          ret = Ok(buf.len() - start_len)
          break
        }
        Ok(n) => {
          len += n
        }
        Err(e) => {
          ret = Err(e)
          break
        }
      }
    }
    ret
  }

  func read_to_str(sb str.Buffer) Result<int, Error> {
    buf := sb.as_shared_bytes()
    read_to_end(buf)
    sb.check_utf8(buf)
    Ok(sb.len())
  }
 */

static Object *read_to_end(Object *self, Object *args)
{
  Slice *buf = array_slice(args);
  int start_len = slice_len(buf);
  int len = slice_len(buf);

  Object *bytes = byte_array_no_buf();
  Slice *slice = array_slice(bytes);
  Object *res, *val;
  int n;

  while (1) {
    if (len == slice_len(buf))
      slice_expand(buf, 32);

    slice_slice_to_end(slice, buf, len);
    res = object_call(self, "read", bytes);
    slice_fini(slice);

    if (!result_test(res))
      break;

    val = result_get_ok(res, NULL);
    OB_DECREF(res);
    n = integer_asint(val);
    OB_DECREF(val);
    if (n <= 0) {
      val = integer_new(len - start_len);
      res = result_ok(val);
      OB_DECREF(val);
      break;
    } else {
      len += n;
    }
  }

  OB_DECREF(bytes);
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

  // restore to string builder
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
  {"read", "[b", RESULT, NULL, 1},
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
  Object *bytes = byte_array_no_buf();
  Slice *slice = array_slice(bytes);
  Slice *buf = string_slice(args);
  slice_slice_to_end(slice, buf, 0);
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
    //printf("nil\n");
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
