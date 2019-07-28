/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_INT_OBJECT_H_
#define _KOALA_INT_OBJECT_H_

#include "object.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct intobject {
  OBJECT_HEAD
  int64_t value;
} IntegerObject;

typedef struct boolobject {
  OBJECT_HEAD
  int value;
} BoolObject;

typedef struct byteobject {
  OBJECT_HEAD
  int value;
} ByteObject;

extern TypeObject Integer_Type;
extern TypeObject Bool_Type;
extern TypeObject Byte_Type;
extern BoolObject True_Object;
extern BoolObject False_Object;
#define Integer_Check(ob) (OB_TYPE(ob) == &Integer_Type)
Object *Integer_New(int64_t val);
Object *Byte_New(int val);
static inline int64_t Integer_AsInt(Object *ob)
{
  if (!Integer_Check(ob)) {
    error("object of '%.64s' is not a type.", OB_TYPE(ob)->name);
    return 0;
  }
  IntegerObject *int_obj = (IntegerObject *)ob;
  return int_obj->value;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INT_OBJECT_H_ */
