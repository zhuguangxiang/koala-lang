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
  Object *owner;
  /* field type descriptor */
  TypeDesc *desc;
  /* getter & setter */
  cfunc get;
  cfunc set;
  /* constant value */
  Object *value;
  /* offset of variable value */
  int offset;
  /* enum value */
  int enumvalue;
} FieldObject;

extern TypeObject Field_Type;
#define Field_Check(ob) (OB_TYPE(ob) == &Field_Type)
Object *Field_New(FieldDef *field);
Object *Field_Get(Object *self, Object *ob);
void Field_Set(Object *self, Object *ob, Object *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */
