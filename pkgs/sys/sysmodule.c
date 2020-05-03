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

#include <stdlib.h>
#include "koala.h"

/*
  var path [string]
  var stdout io.Writer
  var stdin io.Reader
 */

static Object *__path;
static Object *__stdout;
static Object *__stdin;

static Object *__get_path(Object *field, Object *ob)
{
  return OB_INCREF(__path);
}

static int __set_path(Object *self, Object *ob, Object *val)
{
  if (!array_check(val)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(val));
    return -1;
  }

  ArrayObject *arr = (ArrayObject *)val;
  if (!desc_isstr(arr->desc)) {
    STRBUF(buf);
    desc_tostr(arr->desc, &buf);
    error("array's subtype '%s' is not String", strbuf_tostr(&buf));
    strbuf_fini(&buf);
    return -1;
  }

  OB_DECREF(__path);
  __path = OB_INCREF(val);
  return 0;
}

static Object *__get_stdout(Object *field, Object *ob)
{
  return OB_INCREF(__stdout);
}

static int __set_stdout(Object *field, Object *ob, Object *val)
{
  // NOTES: check it is io.Writer
  OB_DECREF(__stdout);
  __stdout = OB_INCREF(val);
  return 0;
}

static Object *__get_stdin(Object *field, Object *ob)
{
  return OB_INCREF(__stdin);
}

static int __set_stdin(Object *field, Object *ob, Object *val)
{
  // NOTES: check it is io.Reader
  OB_DECREF(__stdin);
  __stdin = OB_INCREF(val);
  return 0;
}

static FieldDef sys_vars[] = {
  {"path",   "[s", __get_path, __set_path},
  {"stdout", "Lio.Writer;", __get_stdout, __set_stdout},
  {"stdin",  "Lio.Reader;", __get_stdin, __set_stdin},
  {NULL},
};

static MethodDef sys_funcs[] = {
  {NULL},
};

static void init_sys_path(void)
{
  Object *arr = array_new(&type_base_str);
  __path = arr;
  Object *s = string_new(".");
  array_push_back(arr, s);
  OB_DECREF(s);

  // load KOALA_HOME
  char *home = getenv("KOALA_HOME");
  if (home != NULL) {
    STRBUF(buf);
    strbuf_append(&buf, home);
    strbuf_append(&buf, "/pkgs");
    s = string_new(strbuf_tostr(&buf));
    array_push_back(arr, s);
    OB_DECREF(s);
    strbuf_fini(&buf);
  }

  // load KOALA_PATH
  char *pathes = getenv("KOALA_PATH");
  if (pathes != NULL) {
    char *path = NULL;
    int len = str_sep(&pathes, ':', &path);
    while (len > 0) {
      s = string_with_len(path, len);
      array_push_back(arr, s);
      OB_DECREF(s);
      len = str_sep(&pathes, ':', &path);
    }
  }
}

void init_sys_module(void)
{
  __stdout = file_new(stdout, "stdout");
  __stdin = file_new(stdin, "stdin");
  init_sys_path();

  Object *m = module_new("sys");
  module_add_vardefs(m, sys_vars);
  module_add_funcdefs(m, sys_funcs);
  module_install("sys", m);
  OB_DECREF(m);
}

void fini_sys_module(void)
{
  OB_DECREF(__path);
  OB_DECREF(__stdout);
  OB_DECREF(__stdin);
  module_uninstall("sys");
}

void sys_println(Object *ob)
{
  if (ob == NULL) return;

  Object *s;
  STRBUF(buf);
  if (string_check(ob)) {
    strbuf_append(&buf, string_asstr(ob));
  } else if (integer_check(ob)) {
    strbuf_append_int(&buf, integer_asint(ob));
  } else if (byte_check(ob)) {
    strbuf_append_byte(&buf, byte_asint(ob));
  } else if (float_check(ob)) {
    strbuf_append_float(&buf, float_asflt(ob));
  } else {
    s = object_call(ob, "__str__", NULL);
    strbuf_append(&buf, string_asstr(s));
    OB_DECREF(s);
  }
  strbuf_append(&buf, "\r\n");
  s = string_new(strbuf_tostr(&buf));
  strbuf_fini(&buf);

  // sym_stdout is io.Writer
  Object *out = __get_stdout(NULL, NULL);

  Object *ret = object_call(out, "write_str", s);
  OB_DECREF(s);
  OB_DECREF(ret);

  // release sysm_stdout
  OB_DECREF(out);
}
