/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "moduleobject.h"
#include "fieldobject.h"
#include "stringobject.h"

static Object *_os_path_get_(Object *self, Object *ob)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a field.", OB_TYPE(self)->name);
    return NULL;
  }

  if (!Module_Check(ob)) {
    error("object of '%.64s' is not a module.", OB_TYPE(ob)->name);
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  Object *val = field->value;
  if (val == NULL) {
    val = Array_New_WithType(&String_Type);
    field->value = val;
  }
  return val;
}

static Object *_os_path_set_(Object *self, Object *ob)
{
  if (!Field_Check(self)) {
    error("object of '%.64s' is not a field.", OB_TYPE(self)->name);
    return NULL;
  }

  if (!Tuple_Check(ob)) {
    error("object of '%.64s' is not a Tuple.", OB_TYPE(ob)->name);
    return NULL;
  }

  Object *array = Tuple_Get(ob, 1);
  if (!Array_Check(array)) {
    error("object of '%.64s' is not an Array.", OB_TYPE(array)->name);
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  Object *val = field->value;
  OB_DECREF(val);
  field->value = OB_INCREF(array);
  return NULL;
}

static FieldDef _os_fields_[] = {
  {"path", "Llang.Array;", _os_path_get_, _os_path_set_},
  {NULL}
};

void init_os_module(void)
{
  Object *m = Module_New("os");

  Object *ob;
  int res;
  FieldDef *f = _os_fields_;
  while (f->name != NULL) {
    ob = Field_New(f->name, f->type, f->get, f->set);
    res = Module_Add_Var(m, ob);
    panic(res, "'os' add '%s' failed.", f->name);
    ++f;
  }

  Module_Install("os", m);
}
