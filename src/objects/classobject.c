/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "classobject.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "stringobject.h"

static Object *_class_getfield_(Object *self, Object *name)
{
  if (!Class_Check(self)) {
    error("object of '%.64s' is not a Class.", OB_TYPE(self)->name);
    return NULL;
  }

  if (!String_Check(name)) {
    error("object of '%.64s' is not a String.", OB_TYPE(name)->name);
    return NULL;
  }

  char *namestr = String_AsStr(name);
  ClassObject *cls = (ClassObject *)self;
  Object *res = cls->getfunc(cls->obj, namestr);
  if (!res) {
    error("cannot find field '%s'.", namestr);
    return NULL;
  }

  if (Type_Equal(OB_TYPE(res), &Field_Type)) {
    return res;
  }

  error("'%s' is not a Field.", namestr);
  return NULL;
}

static MethodDef _class_methods_[] = {
  {"getField", "s", "Llang.Field;", _class_getfield_},
  {NULL}
};

TypeObject Class_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Class",
  .getfunc = _object_member_,
  .methods = _class_methods_,
};

Object *Class_New(Object *obj)
{
  ClassObject *clazz = kmalloc(sizeof(*clazz));
  Init_Object_Head(clazz, &Class_Type);
  clazz->obj = OB_INCREF(obj);
  return (Object *)clazz;
}
