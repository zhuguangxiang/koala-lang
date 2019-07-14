/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_INT_OBJECT_H_
#define _KOALA_INT_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct int_object {
  OBJECT_HEAD
  int64_t value;
};

struct bool_object {
  OBJECT_HEAD
  int value;
};

struct byte_object {
  OBJECT_HEAD
  int value;
};

extern struct klass int_type;
extern struct klass bool_type;
extern struct klass byte_type;
extern struct bool_object true_obj;
extern struct bool_object false_obj;
void init_int_type(void);
void free_int_type(void);
struct object *new_integer(int64_t val);
struct object *new_byte(int val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INT_OBJECT_H_ */
