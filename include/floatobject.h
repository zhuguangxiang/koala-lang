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
  double value;
} FloatObject;

extern TypeObject Float_Type;
#define Float_Check(ob) (OB_TYPE(ob) == &Float_Type)
void init_floatobject(void);
void fini_floatobject(void);
Object *Float_New(double val);
static inline double Float_AsFlt(Object *ob)
{
  if (!Float_Check(ob)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(ob));
    return 0;
  }

  FloatObject *fo = (FloatObject *)ob;
  return fo->value;
}

static inline void Float_Set(Object *ob, double f)
{
  if (!Float_Check(ob)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(ob));
    return;
  }

  FloatObject *fo = (FloatObject *)ob;
  fo->value = f;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FLOAT_OBJECT_H_ */
