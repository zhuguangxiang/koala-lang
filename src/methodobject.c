/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "methodobject.h"

static Object *cfunc_call(Object *self, Object *ob, Object *args)
{
  MethodObject *meth = (MethodObject *)self;
  cfunc fn = (cfunc)meth->ptr;
  return fn(ob, args);
}

Object *CMethod_New(MethodDef *def)
{
  MethodObject *method = kmalloc(sizeof(*method));
  Init_Object_Head(method, &Method_Type);
  method->name = def->name;
  method->desc = TypeStr_ToProto(def->ptype, def->rtype);
  method->call = cfunc_call,
  method->ptr = def->func;
  return (Object *)method;
}

TypeObject Method_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Method",
};

TypeObject Proto_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Proto",
};

Object *Method_Call(Object *self, Object *ob, Object *args)
{
  if (!Method_Check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }
  MethodObject *meth = (MethodObject *)self;
  return meth->call(self, ob, args);
}
