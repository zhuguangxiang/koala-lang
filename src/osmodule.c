/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "moduleobject.h"
#include "fieldobject.h"
#include "stringobject.h"
#include "arrayobject.h"

static Object *os_path_get(Object *self, Object *ob)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!Module_Check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  Object *val = field->value;
  if (val == NULL) {
    val = Array_New();
    field->value = val;
  }
  return OB_INCREF(val);
}

static int os_path_set(Object *self, Object *ob, Object *val)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return -1;
  }

  if (!Module_Check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return -1;
  }

  if (!Array_Check(val)) {
    error("object of '%.64s' is not an Array", OB_TYPE_NAME(val));
    return -1;
  }

  FieldObject *field = (FieldObject *)self;
  OB_DECREF(field->value);
  field->value = OB_INCREF(val);
  return 0;
}

static FieldDef os_fields[] = {
  {"path", "Llang.Array;", os_path_get, os_path_set},
  {NULL}
};

void init_os_module(void)
{
  Object *m = Module_New("os");
  Module_Add_VarDefs(m, os_fields);
  Module_Install("os", m);
  OB_DECREF(m);
}

void fini_os_module(void)
{
  Module_Uninstall("os");
}
