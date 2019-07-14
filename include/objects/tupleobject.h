/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_TUPLE_OBJECT_H_
#define _KOALA_TUPLE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tupleobject {
  OBJECT_HEAD
  int size;
  Object *items[0];
} TupleObject;

extern TypeObject tuple_type;
void init_tupleobject(void);
void free_tupleobject(void);
Object *new_tuple(int size);
Object *new_vtuple(int size, ...);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TUPLE_OBJECT_H_ */
