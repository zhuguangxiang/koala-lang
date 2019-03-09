/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "io.h"

LOGGER(0)

static Object *__println(Object *ob, Object *args)
{
  Klass *klazz = OB_KLASS(args);

  if (klazz == &Tuple_Klass) {
    int size = Tuple_Size(args);
    Object *arg;
    for (int i = 0; i < size; i++) {
      arg = Tuple_Get(args, i);
      klazz = OB_KLASS(arg);
      if (klazz->ob_str != NULL) {
        Object *stro = To_String(arg);
        fprintf(stdout, "%s ", String_Raw(stro));
        OB_DECREF(stro);
      } else {
        fprintf(stdout, "%s@%x ", klazz->name, (short)(intptr_t)arg);
      }
    }
  } else {
    if (klazz->ob_str != NULL) {
      Object *stro = To_String(args);
      fprintf(stdout, "%s ", String_Raw(stro));
      OB_DECREF(stro);
    } else {
      fprintf(stdout, "%s@%x ", klazz->name, (short)(intptr_t)args);
    }
  }

  puts(""); /* newline */
  fflush(stdout);
  return NULL;
}

static CFunctionDef io_funcs[] = {
  {"Println", NULL, "...A", __println},
  {NULL}
};

void Init_IO_Package(Package *pkg)
{
  Pkg_Add_CFunctions(pkg, io_funcs);
}

void Fini_IO_Package(void)
{
}
