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

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
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

  FmtterObject *fmt = (FmtterObject *)self;
  StrBuf *sbuf = &fmt->buf;
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
  Object *str = String_New("(");
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

  str = String_New(")");
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
    return String_New(strbuf_tostr(sbuf));
  else
    return NULL;
}

TypeObject Fmtter_Type = {
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
  int res = Type_Ready(&Fmtter_Type);
  if (res != 0)
    panic("Cannot initalize 'Formatter' type.");

  Object *m = Module_New("fmt");
  Module_Add_Type(m, &Fmtter_Type);
  Module_Add_FuncDefs(m, fmt_funcs);
  Module_Install("fmt", m);
}

Object *Fmtter_New(void)
{
  FmtterObject *fmt = kmalloc(sizeof(*fmt));
  Init_Object_Head(fmt, &Fmtter_Type);
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
