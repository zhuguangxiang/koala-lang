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

#include "koala.h"

LOGGER(0)

#define BUF_SIZE 1024

static Object *__print(Object *ob, Object *args)
{
  int size = Tuple_Size(args);
  char *buf = Malloc(BUF_SIZE);
  Object *val;
  int n;
  for (int i = 0; i < size; i++) {
    val = Tuple_Get(args, i);
    n = Object_Print(buf, BUF_SIZE - 1, val);
    fputs(buf, stdout);
    if (i + 1 < size) {
      fputs(" ", stdout); /* space */
    }
  }
  Mfree(buf);
  return NULL;
}

static Object *__println(Object *ob, Object *args)
{
  __print(ob, args);
  fputs("\n", stdout);
  fflush(stdout);
  return NULL;
}

static FuncDef io_funcs[] = {
  {"Println", NULL, "...A", __println},
  {NULL}
};

void Init_IO_Package(void)
{
  Object *pkg = Package_New("io");
  Package_Add_CFunctions(pkg, io_funcs);
}

void Fini_IO_Package(void)
{
}
