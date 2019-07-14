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

struct tuple_object {
  OBJECT_HEAD
  int size;
  struct object *items[0];
};

extern struct klass tuple_type;
void init_tuple_type(void);
void free_tuple_type(void);
struct object *new_tuple(int size);
struct object *new_ntuple(int size, ...);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TUPLE_OBJECT_H_ */
