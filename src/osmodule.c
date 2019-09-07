/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"

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
  ModuleObject *mo = (ModuleObject *)ob;
  Object *val = vector_get(&mo->values, field->offset);
  if (val == NULL) {
    TypeDesc *desc = desc_from_str;
    val = array_new(desc);
    TYPE_DECREF(desc);
    vector_set(&mo->values, field->offset, val);
  }
  return OB_INCREF(val);
}

static FieldDef os_fields[] = {
  {"path", "Llang.Array;", os_path_get, NULL},
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
