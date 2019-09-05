/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "fieldobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "log.h"

Object *Field_New(char *name, TypeDesc *desc)
{
  FieldObject *field = kmalloc(sizeof(*field));
  Init_Object_Head(field, &Field_Type);
  field->name = name;
  field->desc = TYPE_INCREF(desc);
  return (Object *)field;
}

Object *Field_Get(Object *self, Object *ob)
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

int Field_Set(Object *self, Object *ob, Object *val)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return -1;
  }

  FieldObject *field = (FieldObject *)self;
  if (!field->set) {
    error("field '%.64s' is not setable", field->name);
    return -1;
  }

  return field->set(self, ob, val);
}

Object *Field_Default_Get(Object *self, Object *ob)
{
  FieldObject *field = (FieldObject *)self;
  Object *old;
  if (Module_Check(ob)) {
    ModuleObject *mo = (ModuleObject *)ob;
    old = vector_get(&mo->values, field->offset);
    return OB_INCREF(old);
  } else {
    return NULL;
  }
}

int Field_Default_Set(Object *self, Object *ob, Object *val)
{
  FieldObject *field = (FieldObject *)self;
  Object *old;
  if (Module_Check(ob)) {
    ModuleObject *mo = (ModuleObject *)ob;
    old = vector_get(&mo->values, field->offset);
    OB_DECREF(old);
    vector_set(&mo->values, field->offset, OB_INCREF(val));
    return 0;
  } else {
    return -1;
  }
}

static Object *field_set(Object *self, Object *args)
{
  if (args == NULL) {
    error("'set' has no arguments");
    return NULL;
  }

  if (!Tuple_Check(args)) {
    error("arguments of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return NULL;
  }

  Object *ob = Tuple_Get(args, 0);
  Object *val = Tuple_Get(args, 1);

  Field_Set(self, ob, val);
  OB_DECREF(ob);
  OB_DECREF(val);
  return NULL;
}

static void field_free(Object *ob)
{
  if (!Field_Check(ob)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(ob));
    return;
  }

  FieldObject *field = (FieldObject *)ob;
  debug("[Freed] Field '%s'", field->name);
  TYPE_DECREF(field->desc);
  kfree(ob);
}

static MethodDef field_methods[] = {
  {"get", "A",  "A",  Field_Get},
  {"set", "AA", NULL, field_set},
  {NULL}
};

TypeObject Field_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Field",
  .free    = field_free,
  .methods = field_methods,
};
