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
  typedesc *desc;
  /* getter & setter */
  func_t get;
  setfunc set;
  /* offset of value */
  int offset;
  /* enum value */
  int enumvalue;
} FieldObject;

extern TypeObject Field_Type;
#define Field_Check(ob) (OB_TYPE(ob) == &Field_Type)
Object *Field_New(char *name, typedesc *desc);
static inline void Field_SetFunc(Object *self, setfunc set, func_t get)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return;
  }

  FieldObject *field = (FieldObject *)self;
  field->set = set;
  field->get = get;
}

Object *Field_Default_Get(Object *self, Object *ob);
int Field_Default_Set(Object *self, Object *ob, Object *val);

Object *Field_Get(Object *self, Object *ob);
int Field_Set(Object *self, Object *ob, Object *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIELD_OBJECT_H_ */
