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
  TypeDesc *desc;
  Vector items;
} ArrayObject;

extern TypeObject array_type;
#define array_check(ob) (OB_TYPE(ob) == &array_type)
void init_array_type(void);
Object *array_new(TypeDesc *desc);
void array_free(Object *ob);
void Array_Print(Object *ob);
int array_set(Object *self, int index, Object *v);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAY_OBJECT_H_ */
