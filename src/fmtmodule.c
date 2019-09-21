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

Object *Fmtter_WriteFormat(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!Tuple_Check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return NULL;
  }

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
  Object *str = Tuple_Get(args, 0);
  char *fmtstr = string_asstr(str);
  int index = 0;
  char ch;
  Object *ob;

  while ((ch = *fmtstr)) {
    if (ch == '{') {
      ch = *++fmtstr;
      if (ch == '}') {
        ob = Tuple_Get(args, ++index);
        if (ob != NULL) {
          Object_Call(ob, "__fmt__", self);
          OB_DECREF(ob);
        }
      } else {
        strbuf_append_char(sbuf, '{');
        strbuf_append_char(sbuf, ch);
      }
    } else {
      strbuf_append_char(sbuf, ch);
    }
    ++fmtstr;
  }
  OB_DECREF(str);
}

Object *Fmtter_WriteString(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!string_check(args)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(args));
    return NULL;
  }

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
  strbuf_append(sbuf, string_asstr(args));
  return NULL;
}

Object *Fmtter_WriteInteger(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!integer_check(args)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(args));
    return NULL;
  }

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
  IntegerObject *i = (IntegerObject *)args;
  char buf[256];
  sprintf(buf, "%ld", i->value);
  strbuf_append(sbuf, buf);
  return NULL;
}

Object *Fmtter_WriteTuple(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!Tuple_Check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return NULL;
  }

  TupleObject *tuple = (TupleObject *)args;
  Object *str = string_new("(");
  Fmtter_WriteString(self, str);
  OB_DECREF(str);

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
  int size = Tuple_Size(args);
  TUPLE_ITERATOR(iter, tuple);
  Object *item;
  int i = 0;
  iter_for_each(&iter, item) {
    Object_Call(item, "__fmt__", self);
    if (i++ < size - 1)
      strbuf_append(sbuf, ", ");
  }

  str = string_new(")");
  Fmtter_WriteString(self, str);
  OB_DECREF(str);
}

static MethodDef fmtter_methods[] = {
  {"writeFormat",  "s...", NULL, Fmtter_WriteFormat},
  {"writeString",  "s",    NULL, Fmtter_WriteString},
  {"writeInteger", "i",    NULL, Fmtter_WriteInteger},
  {"writeTuple",   "Llang.Tuple;", NULL, Fmtter_WriteTuple},
  {NULL}
};

static Object *fmtter_str(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
  if (sbuf->len > 0)
    return string_new(strbuf_tostr(sbuf));
  else
    return NULL;
}

TypeObject fmtter_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Formatter",
  .free    = Fmtter_Free,
  .str     = fmtter_str,
  .methods = fmtter_methods,
};

void init_fmtter_type(void)
{
  TypeDesc *desc = desc_from_klass("fmt", "Formatter");
  fmtter_type.desc = desc;
  if (type_ready(&fmtter_type) < 0)
    panic("Cannot initalize 'Formatter' type.");
}

static Object *fmt_println(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *fmtter = Fmtter_New();
  Fmtter_WriteFormat(fmtter, args);
  Object *str = fmtter_str(fmtter, NULL);
  OB_DECREF(fmtter);

  IoPrintln(str);
  return NULL;
}

static Object *fmt_print(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *fmtter = Fmtter_New();
  Fmtter_WriteFormat(fmtter, args);
  Object *str = fmtter_str(fmtter, NULL);
  OB_DECREF(fmtter);

  IoPrint(str);
  return NULL;
}

static MethodDef fmt_funcs[] = {
  {"println", "s...", NULL, fmt_println},
  {"print",   "s...", NULL, fmt_print},
  {NULL}
};

void init_fmt_module(void)
{
  Object *m = Module_New("fmt");
  Module_Add_Type(m, &fmtter_type);
  Module_Add_FuncDefs(m, fmt_funcs);
  Module_Install("fmt", m);
  OB_DECREF(m);
}

void fini_fmt_moudle(void)
{
  Module_Uninstall("fmt");
}

Object *Fmtter_New(void)
{
  FmtterObject *fmt = kmalloc(sizeof(*fmt));
  init_object_head(fmt, &fmtter_type);
  return (Object *)fmt;
}

void Fmtter_Free(Object *ob)
{
  if (!Fmtter_Check(ob)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(ob));
    return;
  }

  FmtterObject *fmt = (FmtterObject *)ob;
  StrBuf *sbuf = &fmt->buf;
  strbuf_fini(sbuf);
  kfree(ob);
}
