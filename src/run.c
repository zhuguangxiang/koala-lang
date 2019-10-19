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

#include <pthread.h>
#include "koala.h"
#include "osmodule.h"
#include "iomodule.h"
#include "fmtmodule.h"
#include "gc.h"

static void init_types(void)
{
  print("########\n");

  int res = type_ready(&type_type);
  if (res != 0)
    panic("Cannot initalize 'Type' type.");

  init_any_type();
  init_descob_type();
  init_integer_type();
  init_byte_type();
  init_bool_type();
  init_string_type();
  init_char_type();
  init_float_type();

  init_array_type();
  init_tuple_type();
  init_map_type();
  init_field_type();
  init_method_type();
  init_proto_type();
  init_class_type();
  init_module_type();

  res = type_ready(&code_type);
  if (res != 0)
    panic("Cannot initalize 'Code' type.");
  init_label_type();
  init_range_type();
  init_iter_type();
  init_closure_type();
  init_fmtter_type();

  print("########\n");
}

static void fini_types(void)
{
  print("########\n");
  type_fini(&type_type);
  type_fini(&any_type);
  type_fini(&descob_type);
  type_fini(&byte_type);
  type_fini(&integer_type);
  type_fini(&bool_type);
  type_fini(&string_type);
  type_fini(&char_type);
  type_fini(&float_type);
  type_fini(&array_type);
  type_fini(&tuple_type);
  type_fini(&map_type);
  type_fini(&field_type);
  type_fini(&method_type);
  type_fini(&proto_type);
  type_fini(&class_type);
  type_fini(&module_type);
  type_fini(&code_type);
  type_fini(&label_type);
  type_fini(&range_type);
  type_fini(&iter_type);
  type_fini(&fmtter_type);
  type_fini(&closure_type);
  fini_bool_type();
  print("########\n");
}

void koala_initialize(void)
{
  init_atom();
  init_typedesc();
  init_types();
  init_lang_module();
  init_os_module();
  init_io_module();
  init_fmt_module();
  init_assert_module();
  init_parser();
}

void koala_finalize(void)
{
  fini_parser();
  fini_assert_module();
  fini_fmt_moudle();
  fini_io_module();
  fini_os_module();
  fini_lang_module();
  gc();
  fini_types();
  fini_typedesc();
  fini_atom();
}

/*
static void run_klc(char *klc)
{
  if (exist(klc)) {
    run();
    return;
  }

  char *kl = str_ndup(klc, strlen(klc) - 1);
  if (exist(kl)) {
    compile();
    run();
    kfree(kl);
    return;
  }

  char *dir = str_ndup(klc, strlen(klc) - 4);
  if (exist(dir)) {
    compile();
    run();
    kfree(dir);
    return;
  }

  error();
}

static void run_kl(char *kl)
{
  char *klc;
  if (exist(klc)) {
    return;
  }

  if (!exist(kl)) {
    return;
  }

  compile();
  run_klc(klc);
}

static void run_dir(char *dir)
{

}
*/

/*
 * The following commands are valid.
 * ~$ koala a/b/foo.kl [a/b/foo.klc] [a/b/foo]
 */
void koala_execute(char *path)
{
  /*
  if (isdotklc(path)) {
    run_klc(path);
  } else if (isdotkl(path)) {
    run_kl(path);
  } else {
    run_dir(path);
  }
  */
}
