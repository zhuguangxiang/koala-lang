/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_FIELD_OBJECT_H_
#define _KOALA_FIELD_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fieldobject {
  OBJECT_HEAD
  /* field name */
  char *name;
  /* field owner */
  TypeObject *owner;
  /* field type descriptor */
  TypeDesc *desc;
  /* getter & setter */
  getfunc getfunc;
  setfunc setfunc;
  /* constant value */
  Object *value;
  /* offset of variable value */
  int offset;
  /* enum value */
  int enumvalue;
} FieldObject;

extern TypeObject Field_Type;
#define Field_Check(ob) (OB_TYPE(ob) == &Field_Type)
Object *Field_New(char *name, char *type, getfunc get, setfunc set);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */
