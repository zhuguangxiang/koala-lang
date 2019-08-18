/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "methodobject.h"
#include "tupleobject.h"

static Object *cfunc_call(Object *self, Object *ob, Object *args)
{
  MethodObject *meth = (MethodObject *)self;
  func_t fn = (func_t)meth->ptr;
  return fn(ob, args);
}

Object *CMethod_New(MethodDef *def)
{
  MethodObject *method = kmalloc(sizeof(*method));
  Init_Object_Head(method, &Method_Type);
  method->name = def->name;
  method->desc = string_toproto(def->ptype, def->rtype);
  method->call = cfunc_call,
  method->ptr = def->func;
  return (Object *)method;
}

static Object *method_call(Object *self, Object *args)
{
  if (!Method_Check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }
  if (!args)
    panic("'call' has no arguments");

  Object *ob;
  Object *para;
  if (!Tuple_Check(args)) {
    ob = OB_INCREF(args);
    para = NULL;
  } else {
    ob = Tuple_Get(args, 0);
    para = Tuple_Slice(args, 1, -1);
  }

  MethodObject *meth = (MethodObject *)self;
  Object *res = meth->call(self, ob, args);
  OB_DECREF(ob);
  OB_DECREF(para);
  return res;
}

static void meth_free(Object *ob)
{
  if (!Method_Check(ob)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(ob));
    return;
  }
  MethodObject *meth = (MethodObject *)ob;
  debug("[Freed] Method %s", meth->name);
  TYPE_DECREF(meth->desc);
  kfree(ob);
}

static MethodDef meth_methods[] = {
  {"call", "...", "A", method_call},
  {NULL}
};

TypeObject Method_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Method",
  .free    = meth_free,
  .methods = meth_methods,
};

TypeObject Proto_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Proto",
};
