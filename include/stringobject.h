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

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRING_OBJECT_H_ */
