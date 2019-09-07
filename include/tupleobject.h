/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_TUPLE_OBJECT_H_
#define _KOALA_TUPLE_OBJECT_H_

#include "object.h"
#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tupleobject {
  OBJECT_HEAD
  int size;
  Object *items[0];
} TupleObject;

extern TypeObject tuple_type;
#define Tuple_Check(ob) (OB_TYPE(ob) == &tuple_type)
void init_tuple_type(void);
Object *Tuple_New(int size);
Object *Tuple_Pack(int size, ...);
int Tuple_Size(Object *self);
Object *Tuple_Get(Object *self, int index);
int Tuple_Set(Object *self, int index, Object *val);
Object *Tuple_Slice(Object *self, int i, int j);
void *tuple_iter_next(struct iterator *iter);
#define TUPLE_ITERATOR(name, tuple) \
  ITERATOR(name, tuple, tuple_iter_next)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TUPLE_OBJECT_H_ */
