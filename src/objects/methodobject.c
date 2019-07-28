/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "methodobject.h"
#include "log.h"

Object *CMethod_New(char *name, char *ptype, char *rtype, cfunc func)
{
  MethodObject *method = kmalloc(sizeof(*method));
  Init_Object_Head(method, &Method_Type);
  method->name = name;
  method->desc = TypeStr_ToProto(ptype, rtype);
  method->cfunc = 1;
  method->ptr = func;
  return (Object *)method;
}

static Object *_method_get_cb_(Object *self, Object *ob)
{
  if (!Method_Check(self)) {
    error("object of '%.64s' is not a method.", OB_TYPE(self)->name);
    return NULL;
  }
  return self;
}

TypeObject Method_Type = {
  OBJECT_HEAD_INIT(&Class_Type)
  .name = "Method",
  .getfunc = _method_get_cb_,
};

TypeObject Proto_Type = {
  OBJECT_HEAD_INIT(&Class_Type)
  .name = "Proto",
};
