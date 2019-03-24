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

Object *IO_Puts(Object *ob, Object *args)
{
  OB_ASSERT_KLASS(args, String_Klass);
  puts(String_Raw(args));
  fflush(stdout);
  return NULL;
}

static CFuncDef io_funcs[] = {
  {"puts", "s", NULL, IO_Puts},
  {NULL}
};

void Init_IO_Package(void)
{
  Object *pkg = New_Package(KOALA_PKG_IO);
  Pkg_Add_CFuns(pkg, io_funcs);
  Install_Package(KOALA_PKG_IO, pkg);
}

void Fini_IO_Package(void)
{
}
