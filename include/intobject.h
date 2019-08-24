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
#define Integer_Check(ob) (OB_TYPE(ob) == &Integer_Type)
Object *Integer_New(int64_t val);
static inline int64_t Integer_AsInt(Object *ob)
{
  if (!Integer_Check(ob)) {
    error("object of '%.64s' is not a Integer.", OB_TYPE(ob)->name);
    return 0;
  }
  IntegerObject *int_obj = (IntegerObject *)ob;
  return int_obj->value;
}

extern TypeObject Byte_Type;
#define Byte_Check(ob) (OB_TYPE(ob) == &Byte_Type)
Object *Byte_New(int val);
static inline int Byte_AsInt(Object *ob)
{
  if (!Byte_Check(ob)) {
    error("object of '%.64s' is not a Byte.", OB_TYPE(ob)->name);
    return 0;
  }
  ByteObject *byte_obj = (ByteObject *)ob;
  return byte_obj->value;
}

extern TypeObject Bool_Type;
#define Bool_Check(ob) (OB_TYPE(ob) == &Bool_Type)
extern BoolObject OB_True;
extern BoolObject OB_False;

static inline Object *Bool_True(void)
{
  Object *res = (Object *)&OB_True;
  return OB_INCREF(res);
}

static inline Object *Bool_False(void)
{
  Object *res = (Object *)&OB_False;
  return OB_INCREF(res);
}

static inline int Bool_IsTrue(Object *ob)
{
  if (!Bool_Check(ob)) {
    error("object of '%.64s' is not a Bool.", OB_TYPE(ob)->name);
    return 0;
  }
  return ob == (Object *)&OB_True ? 1 : 0;
}

static inline int Bool_IsFalse(Object *ob)
{
  if (!Bool_Check(ob)) {
    error("object of '%.64s' is not a Bool.", OB_TYPE(ob)->name);
    return 0;
  }
  return ob == (Object *)&OB_False ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INT_OBJECT_H_ */
