/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_ARRAY_OBJECT_H_
#define _KOALA_ARRAY_OBJECT_H_

#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arrayobject {
  OBJECT_HEAD
  TypeObject *type;
  Vector items;
} ArrayObject;

extern TypeObject Array_Type;
#define Array_Check(ob) (OB_TYPE(ob) == &Array_Type)
Object *Array_New(void);
void Array_Free(Object *ob);
void Array_Print(Object *ob);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAY_OBJECT_H_ */
