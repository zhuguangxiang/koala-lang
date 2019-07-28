/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_FLOAT_OBJECT_H_
#define _KOALA_FLOAT_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct floatobject {
  OBJECT_HEAD
} FloatObject;

extern TypeObject float_type;
void init_floatobject(void);
void fini_floatobject(void);
Object *new_float(double val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FLOAT_OBJECT_H_ */
