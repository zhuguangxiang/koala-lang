/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_CLASS_OBJECT_H_
#define _KOALA_CLASS_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct classobject {
  OBJECT_HEAD
  Object *obj;
} ClassObject;

extern TypeObject class_type;
#define Class_Check(ob) (OB_TYPE(ob) == &class_type)
void init_class_type(void);
Object *Class_New(Object *obj);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CLASS_OBJECT_H_ */
