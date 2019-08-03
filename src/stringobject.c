/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "stringobject.h"
#include "hashmap.h"
#include "intobject.h"
#include "log.h"

static Object *string_concat(Object *self, Object *args)
{
  return NULL;
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

static MethodDef string_methods[] = {
  {"concat",  "s",  "s", string_concat},
  {"length",  NULL, "i", string_length},
  {"__add__", "s",  "s", string_concat},
  {NULL}
};

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

  return strcmp(s1->wstr, s2->wstr) ? Bool_True() : Bool_False();
}

static Object *string_str(Object *self, Object *args)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }
  return OB_INCREF(self);
}

TypeObject String_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "String",
  .hash    = string_hash,
  .equal   = string_equal,
  .str     = string_str,
  .lookup  = Object_Lookup,
  .methods = string_methods,
};

Object *String_New(char *str)
{
  int len = strlen(str);
  StringObject *string = kmalloc(sizeof(*string) + len + 1);
  Init_Object_Head(string, &String_Type);
  string->len = len;
  string->wstr = (char *)(string + 1);
  strcpy(string->wstr, str);
  return (Object *)string;
}

char *String_AsStr(Object *self)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  StringObject *string = (StringObject *)self;
  return string->wstr;
}
