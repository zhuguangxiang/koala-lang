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

#include "fmt.h"
#include "io.h"

void Format_WriteFormat(Object *ob, Object *args)
{
  Formatter *fmtter = (Object *)ob;
  Object *o = Tuple_Get(args, 0);
  char *s = String_Raw(o);
  int i = 1;
  char ch;
  while ((ch = *s++) != '\0') {
    if ((ch == '{') && (ch = *s++) == '}') {
      o = Tuple_Get(args, i++);
      Koala_Run(ob, "__display__", ob);
    } else {
      StringBuf_Append_Char(fmtter->buf, ch);
    }
  }
}

void Format_WriteString(Object *ob, char *str)
{
  Formatter *fmtter = (Object *)ob;
  StringBuf_Append_CStr(fmtter->buf, str);
}

Object *Format_ToString(Object *ob)
{
  Formatter *fmtter = (Object *)ob;
  return String_New(fmtter->buf.data);
}

static Object *__write_string(Object *ob, Object *args)
{
  char *str = String_Raw(args);
  Format_WriteString(ob, str);
  return NULL;
}

static Object *__write_format(Object *ob, Object *args)
{
  Format(ob, args);
  return NULL
}

static Object *__to_string(Object *ob, Object *args)
{
  return Format_ToString(ob);
}

static CFuncDef format_funcs[] = {
  {"writeString", "s",     NULL, __write_string},
  {"writeFormat", "s...A", NULL, __write_format},
  {"toString",    NULL,    "s",  __to_string},
  {NULL}
};

Klass Format_Klass = {
  OBJECT_HEAD_INIT(&Class_Klass)
  .name = "Formatter"
};

void Init_Format_Package(void)
{
  Object *pkg = Find_Package(KOALA_PKG_FMT);
  Klass *klazz = Pkg_Get_Class(pkg, "Formatter");
  Klass_Add_CMethods(klazz, format_funcs);
}

void Fini_Format_Package(void)
{
}
