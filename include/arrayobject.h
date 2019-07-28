/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_ARRAY_OBJECT_H_
#define _KOALA_ARRAY_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arrayobject {
  OBJECT_HEAD
} ArrayObject;

extern TypeObject array_type;
void init_arrayobject(void);
void fini_arrayobject(void);
Object *new_array(int dims, TypeDesc *type);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAY_OBJECT_H_ */
