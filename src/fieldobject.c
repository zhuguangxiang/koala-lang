/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "fieldobject.h"
#include "log.h"

Object *Field_New(FieldDef *def)
{
  FieldObject *field = kmalloc(sizeof(*field));
  Init_Object_Head(field, &Field_Type);
  field->name = def->name;
  field->desc = TypeStr_ToDesc(def->type);
  field->get = def->get;
  field->set = def->set;
  return (Object *)field;
}

static Object *field_get(Object *self, Object *ob)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  if (!field->get) {
    error("field '%.64s' is not getable", field->name);
    return NULL;
  }

  return field->get(self, ob);
}

static Object *field_set(Object *self, Object *args)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return 0;
  }

  FieldObject *field = (FieldObject *)self;
  if (!field->set) {
    error("field '%.64s' is not setable", field->name);
    return 0;
  }

  return field->set(self, args);
}

static MethodDef field_methods[] = {
  {"get", "A",  "A",  field_get},
  {"set", "AA", NULL, field_set},
  {NULL}
};

TypeObject Field_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Field",
  .lookup  = Object_Lookup,
  .methods = field_methods,
};
