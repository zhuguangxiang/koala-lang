/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "fieldobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "log.h"

Object *field_new(char *name, TypeDesc *desc)
{
  FieldObject *field = kmalloc(sizeof(*field));
  init_object_head(field, &field_type);
  field->name = name;
  field->desc = TYPE_INCREF(desc);
  return (Object *)field;
}

Object *field_get(Object *self, Object *ob)
{
  if (!field_check(self)) {
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

int field_set(Object *self, Object *ob, Object *val)
{
  if (!field_check(self)) {
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

Object *field_default_getter(Object *self, Object *ob)
{
  FieldObject *field = (FieldObject *)self;
  int index = field->offset;
  Object *old;
  if (Module_Check(ob)) {
    ModuleObject *mo = (ModuleObject *)ob;
    int size = vector_size(&mo->values);
    if (index < 0 || index >= size) {
      error("index %d out of range(0..<%d)", index, size);
      return NULL;
    }
    old = vector_get(&mo->values, index);
    return OB_INCREF(old);
  } else {
    return NULL;
  }
}

int field_default_setter(Object *self, Object *ob, Object *val)
{
  FieldObject *field = (FieldObject *)self;
  int index = field->offset;
  Object *old;
  if (Module_Check(ob)) {
    ModuleObject *mo = (ModuleObject *)ob;
    int size = vector_size(&mo->values);
    if (index < 0 || index > size) {
      error("index %d out of range(0...%d)", index, size);
      return -1;
    }
    old = vector_set(&mo->values, field->offset, OB_INCREF(val));
    OB_DECREF(old);
    return 0;
  } else {
    return -1;
  }
}

static Object *_field_set_(Object *self, Object *args)
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

  field_set(self, ob, val);
  OB_DECREF(ob);
  OB_DECREF(val);
  return NULL;
}

static void field_free(Object *ob)
{
  if (!field_check(ob)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(ob));
    return;
  }

  FieldObject *field = (FieldObject *)ob;
  debug("[Freed] Field '%s'", field->name);
  TYPE_DECREF(field->desc);
  kfree(ob);
}

static Object *field_str(Object *self, Object *ob)
{
  if (!field_check(self)) {
    error("object of '%.64s' is not a Field", OB_TYPE_NAME(self));
    return NULL;
  }

  FieldObject *field = (FieldObject *)self;
  return string_new(field->name);
}

static MethodDef field_methods[] = {
  {"get", "A",  "A",  field_get},
  {"set", "AA", NULL, _field_set_},
  {NULL}
};

TypeObject field_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Field",
  .free    = field_free,
  .str     = field_str,
  .methods = field_methods,
};

void init_field_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Field");
  field_type.desc = desc;
  if (type_ready(&field_type) < 0)
    panic("Cannot initalize 'Field' type.");
}
