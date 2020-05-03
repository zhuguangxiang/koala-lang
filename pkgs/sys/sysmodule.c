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
  var stdout io.Writer
  var stdin io.Reader
  var path [string]
 */

static Object *sys_stdout;
static Object *sys_stdin;
static Object *sys_path;

static Object *__path(void)
{
  return sys_path;
}

static Object *__stdout(void)
{
  return sys_stdout;
}

static Object *__stdin(void)
{
  return sys_stdin;
}

static FieldDef sys_vars[] = {
  {
    "path",   "[s",
    field_default_getter,
    NULL,
    __path
  },
  {
    "stdin",  "Lio.Reader;",
    field_default_getter,
    field_default_setter,
    __stdin
  },
  {
    "stdout", "Lio.Writer;",
    field_default_getter,
    field_default_setter,
    __stdout
  },
  {NULL},
};

static MethodDef sys_funcs[] = {
  {NULL},
};

static void init_sys_path(void)
{
  Object *arr = array_new(&type_base_str);
  sys_path = arr;
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
  sys_stdout = file_new(stdout, "stdout");
  sys_stdin = file_new(stdin, "stdin");
  init_sys_path();

  Object *m = module_new("sys");
  module_add_vardefs(m, sys_vars);
  module_add_funcdefs(m, sys_funcs);
  module_install("sys", m);
  OB_DECREF(m);
}

void fini_sys_module(void)
{
  module_uninstall("sys");
}
