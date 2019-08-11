/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "classobject.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "stringobject.h"

static Object *class_get(Object *self, char *name)
{
  if (!Class_Check(self)) {
    error("object of '%.64s' is not a Class", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *res;
  Object *ob = ((ClassObject *)self)->obj;
  if (Type_Check(ob)) {
    res = Type_Lookup((TypeObject *)ob, name);
  } else {
    res = Object_Lookup(ob, name);
  }

  if (!res) {
    error("cannot find '%.64s'", name);
    return NULL;
  }

  return OB_INCREF(res);
}

static Object *class_getfield(Object *self, Object *name)
{
  if (!String_Check(name)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(name));
    return NULL;
  }

  char *s = String_AsStr(name);
  Object *ob = class_get(self, s);
  if (ob == NULL)
    return NULL;

  if (!Field_Check(ob)) {
    error("'%s' is not a Field", s);
    OB_DECREF(ob);
    return NULL;
  }

  return ob;
}

static Object *class_getmethod(Object *self, Object *name)
{
  if (!String_Check(name)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(name));
    return NULL;
  }

  char *s = String_AsStr(name);
  Object *ob = class_get(self, s);
  if (ob == NULL)
    return NULL;

  if (!Method_Check(ob)) {
    error("'%s' is not a Method", s);
    OB_DECREF(ob);
    return NULL;
  }

  return ob;
}

static Object *class_name(Object *self, Object *args)
{
  if (!Class_Check(self)) {
    error("object of '%.64s' is not a Class", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *ob = ((ClassObject *)self)->obj;
  if (OB_TYPE(ob) == &Type_Type) {
    return String_New(((TypeObject *)ob)->name);
  } else {
    return String_New(OB_TYPE_NAME(ob));
  }
}

static Object *class_module(Object *self, Object *args)
{
  if (!Class_Check(self)) {
    error("object of '%.64s' is not a Class", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *res;
  Object *ob = ((ClassObject *)self)->obj;
  if (OB_TYPE(ob) == &Type_Type) {
    res = ((TypeObject *)ob)->owner;
  } else {
    res = OB_TYPE(ob)->owner;
    if (res == NULL)
      res = ob;
  }

  return OB_INCREF(res);
}

static Object *class_members(Object *self, Object *args)
{

}

static Object *class_lro(Object *self, Object *args)
{

}

static MethodDef class_methods[] = {
  {"__name__",   NULL, "s",             class_name     },
  {"__module__", NULL, "Llang.Module;", class_module   },
  {"__lro__",    NULL, "Llang.Tuple;",  class_lro      },
  {"__mbrs__",   NULL, "Llang.Tuple;",  class_members  },
  {"getField",   "s",  "Llang.Field;",  class_getfield },
  {"getMethod",  "s",  "Llang.Method;", class_getmethod},
  {NULL}
};

TypeObject Class_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Class",
  .methods = class_methods,
};

Object *Class_New(Object *obj)
{
  ClassObject *clazz = kmalloc(sizeof(*clazz));
  Init_Object_Head(clazz, &Class_Type);
  clazz->obj = OB_INCREF(obj);
  return (Object *)clazz;
}
