/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_

#include "common.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stringobject {
  OBJECT_HEAD
  /* unicode length */
  int len;
  char *wstr;
} StringObject;

typedef struct charobject {
  OBJECT_HEAD
  unsigned int value;
} CharObject;

extern TypeObject String_Type;
#define String_Check(ob) (OB_TYPE(ob) == &String_Type)
Object *String_New(char *str);
char *String_AsStr(Object *self);
int String_IsEmpty(Object *self);
void String_Set(Object *self, char *str);
extern TypeObject Char_Type;
#define Char_Check(ob) (OB_TYPE(ob) == &Char_Type)
Object *Char_New(unsigned int val);
static inline int Char_AsChar(Object *ob)
{
  if (Char_Check(ob)) {
    error("object of '%.64s' is not a Char.", OB_TYPE(ob)->name);
    return 0;
  }

  CharObject *ch = (CharObject *)ob;
  return ch->value;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
