/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "stringobject.h"
#include "hashmap.h"
#include "intobject.h"
#include "log.h"

static Object *_string_concat_cb_(Object *self, Object *args)
{
  return NULL;
}

static Object *_string_length_cb_(Object *self, Object *args)
{
  return NULL;
}

static MethodDef string_methods[] = {
  {"concat", "s", "s", _string_concat_cb_},
  {"length", NULL, "i", _string_length_cb_},
  {"__add__", "s", "s", _string_concat_cb_},
  {NULL}
};

static Object *_string_hash_cb_(Object *self, Object *args)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a string.", OB_TYPE(self)->name);
    return NULL;
  }

  return Integer_New(strhash(String_AsStr(self)));
}

static Object *_string_cmp_cb_(Object *self, Object *other)
{
  if (!String_Check(self)) {
    error("object of '%.64s' is not a string.", OB_TYPE(self)->name);
    return NULL;
  }

  if (!String_Check(other)) {
    error("object of '%.64s' is not a string.", OB_TYPE(other)->name);
    return NULL;
  }

  if (self == other)
    return 0;

  StringObject *s1 = (StringObject *)self;
  StringObject *s2 = (StringObject *)other;

  return Integer_New(strcmp(s1->wstr, s2->wstr));
}

TypeObject String_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "String",
  .hashfunc  = _string_hash_cb_,
  .equalfunc = _string_cmp_cb_,
  .methods   = string_methods,
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
    error("object of '%.64s' is not a string.", OB_TYPE(self)->name);
    return NULL;
  }

  StringObject *string = (StringObject *)self;
  return string->wstr;
}
