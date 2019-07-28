/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "fieldobject.h"
#include "log.h"

Object *Field_New(char *name, char *type, getfunc get, setfunc set)
{
  FieldObject *field = kmalloc(sizeof(*field));
  Init_Object_Head(field, &Field_Type);
  field->name = name;
  field->desc = TypeStr_ToDesc(type);
  field->getfunc = get;
  field->setfunc = set;
  return (Object *)field;
}

static Object *_field_get_cb_(Object *self, Object *ob)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a field.", OB_TYPE(self)->name);
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  if (!field->getfunc) {
    error("field of '%.64s' is not getterable.", OB_TYPE(ob)->name);
    return NULL;
  }

  return field->getfunc(self, ob);
}

static int _field_set_cb_(Object *self, Object *ob, Object *arg)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a field.", OB_TYPE(self)->name);
    return 0;
  }

  FieldObject *field = (FieldObject *)self;
  if (!field->setfunc) {
    error("field of '%.64s' is not settable.", OB_TYPE(ob)->name);
    return 0;
  }

  return field->setfunc(self, ob, arg);
}

TypeObject Field_Type = {
  OBJECT_HEAD_INIT(&Class_Type)
  .name = "Field",
  .getfunc = _field_get_cb_,
  .setfunc = _field_set_cb_,
};
