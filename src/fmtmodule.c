/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "fmtmodule.h"
#include "moduleobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "iomodule.h"
#include "intobject.h"

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

  FormatterObject *fmt = (FormatterObject *)self;
  struct strbuf *sbuf = &fmt->buf;
  Object *str = Tuple_Get(args, 0);
  char *fmtstr = String_AsStr(str);
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

  if (!String_Check(args)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(args));
    return NULL;
  }

  FormatterObject *fmt = (FormatterObject *)self;
  struct strbuf *sbuf = &fmt->buf;
  strbuf_append(sbuf, String_AsStr(args));
  return NULL;
}

Object *Fmtter_WriteInteger(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!Integer_Check(args)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(args));
    return NULL;
  }

  FormatterObject *fmt = (FormatterObject *)self;
  struct strbuf *sbuf = &fmt->buf;
  IntegerObject *i = (IntegerObject *)args;
  char buf[256];
  sprintf(buf, "%ld", i->value);
  strbuf_append(sbuf, buf);
  return NULL;
}

static MethodDef fmtter_methods[] = {
  {"writeformat", "s...", NULL, Fmtter_WriteFormat},
  {"writestring", "s",    NULL, Fmtter_WriteString},
  {"writeint",    "i",    NULL, Fmtter_WriteInteger},
  {NULL}
};

static Object *fmtter_str(Object *self, Object *args)
{
  if (!Fmtter_Check(self)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(self));
    return NULL;
  }

  FormatterObject *fmt = (FormatterObject *)self;
  struct strbuf *sbuf = &fmt->buf;
  if (sbuf->len > 0)
    return String_New(strbuf_tostr(sbuf));
  else
    return NULL;
}

TypeObject Formatter_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Formatter",
  .free    = Fmtter_Free,
  .str     = fmtter_str,
  .methods = fmtter_methods,
};

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

  io_putln(str);
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

  io_put(str);
  return NULL;
}

static MethodDef fmt_funcs[] = {
  {"println", "s...", NULL, fmt_println},
  {"print",   "s...", NULL, fmt_print},
  {NULL}
};

void init_fmt_module(void)
{
  int res = Type_Ready(&Formatter_Type);
  panic(res, "Cannot initalize 'Formatter' type.");

  Object *m = Module_New("fmt");
  Module_Add_Type(m, &Formatter_Type);
  Module_Add_FuncDefs(m, fmt_funcs);
  Module_Install("fmt", m);
}

Object *Fmtter_New(void)
{
  FormatterObject *fmt = kmalloc(sizeof(*fmt));
  Init_Object_Head(fmt, &Formatter_Type);
  return (Object *)fmt;
}

void Fmtter_Free(Object *ob)
{
  if (!Fmtter_Check(ob)) {
    error("object of '%.64s' is not a Formatter", OB_TYPE_NAME(ob));
    return;
  }

  FormatterObject *fmt = (FormatterObject *)ob;
  struct strbuf *sbuf = &fmt->buf;
  strbuf_free(sbuf);
  kfree(ob);
}
