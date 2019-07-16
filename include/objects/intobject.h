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

typedef struct intobject {
  OBJECT_HEAD
  int64_t value;
} IntObject;

typedef struct boolobject {
  OBJECT_HEAD
  int value;
} BoolObject;

typedef struct byteobject {
  OBJECT_HEAD
  int value;
} ByteObject;

extern Klass int_type;
extern Klass bool_type;
extern Klass byte_type;
extern BoolObject true_object;
extern BoolObject false_object;
void init_intobject(void);
void fini_intobject(void);
Object *new_integer(int64_t val);
Object *new_byte(int val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_INT_OBJECT_H_ */
