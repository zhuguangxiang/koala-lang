/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "stringobject.h"
#include "hashmap.h"
#include "intobject.h"
#include "fmtmodule.h"
#include "utf8.h"
#include "log.h"

static Object *str_num_add(Object *x, Object *y)
{
  if (!String_Check(x)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!String_Check(y)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(y));
    return NULL;
  }

  Object *z;
  STRBUF(sbuf);
  strbuf_append(&sbuf, String_AsStr(x));
  strbuf_append(&sbuf, String_AsStr(y));
  z = String_New(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  return z;
}

static Object *string_length(Object *self, Object *args)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  if (args != NULL) {
    error("length() of 'String' has no arguments");
    return NULL;
  }

  StringObject *s = (StringObject *)self;
  return Integer_New(s->len);
}

static Object *string_fmt(Object *self, Object *ob)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  STRBUF(sbuf);
  strbuf_append_char(&sbuf, '\'');
  strbuf_append(&sbuf, String_AsStr(self));
  strbuf_append_char(&sbuf, '\'');
  Object *str = String_New(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  Fmtter_WriteString(ob, str);
  OB_DECREF(str);
  return NULL;
}

static Object *string_hash(Object *self, Object *args)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  return Integer_New(strhash(String_AsStr(self)));
}

static Object *string_equal(Object *self, Object *other)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!String_Check(other)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(other));
    return NULL;
  }

  if (self == other)
    return Bool_True();

  StringObject *s1 = (StringObject *)self;
  StringObject *s2 = (StringObject *)other;

  return !strcmp(s1->wstr, s2->wstr) ? Bool_True() : Bool_False();
}

static void string_free(Object *self)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return;
  }
  debug("[Freed] String '%.64s'", String_AsStr(self));
  StringObject *s = (StringObject *)self;
  kfree(s->wstr);
  kfree(self);
}

static Object *string_str(Object *self, Object *args)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }
  return OB_INCREF(self);
}

static int str_num_cmp(Object *x, Object *y)
{
  if (!String_Check(x)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(x));
    return -1;
  }

  if (!String_Check(y)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(y));
    return -1;
  }

  char *s1 = String_AsStr(x);
  char *s2 = String_AsStr(y);
  return strcmp(s1, s2);
}

static Object *str_num_gt(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r > 0) ? Bool_True() : Bool_False();
}

static Object *str_num_ge(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r >= 0) ? Bool_True() : Bool_False();
}

static Object *str_num_lt(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r < 0) ? Bool_True() : Bool_False();
}

static Object *str_num_le(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r < 0) ? Bool_True() : Bool_False();
}

static Object *str_num_eq(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r == 0) ? Bool_True() : Bool_False();
}

static Object *str_num_neq(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r != 0) ? Bool_True() : Bool_False();
}

static Object *str_num_inadd(Object *x, Object *y)
{
  if (!String_Check(x)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!String_Check(y)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(y));
    return NULL;
  }

  STRBUF(sbuf);
  strbuf_append(&sbuf, String_AsStr(x));
  strbuf_append(&sbuf, String_AsStr(y));
  String_Set(x, strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  return NULL;
}

static MethodDef string_methods[] = {
  {"concat",  "s",  "s", str_num_add},
  {"length",  NULL, "i", string_length},
  {"__fmt__", "Lfmt.Formatter;", NULL, string_fmt},
  {"__add__", "s", "s", str_num_add},
  {"__gt__", "s", "z", str_num_gt},
  {"__ge__", "s", "z", str_num_ge},
  {"__lt__", "s", "z", str_num_lt},
  {"__le__", "s", "z", str_num_le},
  {"__eq__", "s", "z", str_num_eq},
  {"__neq__", "s", "z", str_num_neq},
  {"__inadd__", "ss", NULL, str_num_inadd},
  {NULL}
};

TypeObject String_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "String",
  .hash    = string_hash,
  .equal   = string_equal,
  .free    = string_free,
  .methods = string_methods,
};

void init_string_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "String");
  String_Type.desc = desc;
  if (type_ready(&String_Type) < 0)
    panic("Cannot initalize 'String_Type' type.");
}

Object *String_New(char *str)
{
  int len = strlen(str);
  StringObject *s = kmalloc(sizeof(*s));
  init_object_head(s, &String_Type);
  s->len = len;
  s->wstr = kmalloc(len + 1);
  strcpy(s->wstr, str);
  return (Object *)s;
}

char *String_AsStr(Object *self)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  StringObject *s = (StringObject *)self;
  return s->wstr;
}

int String_IsEmpty(Object *self)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return 0;
  }

  StringObject *s = (StringObject *)self;
  if (s->wstr == NULL)
    return 1;
  if (strlen(s->wstr) == 0)
    return 1;
  return 0;
}

void String_Set(Object *self, char *str)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return;
  }

  StringObject *s = (StringObject *)self;
  kfree(s->wstr);
  s->wstr = kmalloc(strlen(str) + 1);
  strcpy(s->wstr, str);
}

static void char_free(Object *ob)
{
  if (!Char_Check(ob)) {
    error("object of '%.64s' is not a Char", OB_TYPE_NAME(ob));
    return;
  }
  CharObject *ch = (CharObject *)ob;
  debug("[Freed] Char %s", (char *)&ch->value);
  kfree(ob);
}

static Object *char_str(Object *self, Object *ob)
{
  if (!Char_Check(self)) {
    error("object of '%.64s' is not a Char", OB_TYPE_NAME(self));
    return NULL;
  }

  CharObject *ch = (CharObject *)self;
  char buf[8] = {'\'', 0};
  int bytes = encode_one_utf8_char(ch->value, buf + 1);
  buf[bytes + 1] = '\'';
  return String_New(buf);
}

TypeObject Char_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Character",
  .free    = char_free,
  .str     = char_str,
};

void init_char_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Character");
  Char_Type.desc = desc;
  if (type_ready(&Char_Type) < 0)
    panic("Cannot initalize 'Character' type.");
}

Object *Char_New(unsigned int val)
{
  CharObject *ch = kmalloc(sizeof(*ch));
  init_object_head(ch, &Char_Type);
  ch->value = val;
  return (Object *)ch;
}