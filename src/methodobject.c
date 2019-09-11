/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "methodobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "codeobject.h"
#include "eval.h"

Object *CMethod_New(MethodDef *def)
{
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name  = def->name;
  method->desc  = str_to_proto(def->ptype, def->rtype);
  method->cfunc = 1,
  method->ptr   = def->func;
  return (Object *)method;
}

Object *Method_New(char *name, Object *code)
{
  CodeObject *co = (CodeObject *)code;
  MethodObject *method = kmalloc(sizeof(*method));
  init_object_head(method, &method_type);
  method->name = name;
  method->desc = TYPE_INCREF(co->proto);
  method->ptr  = OB_INCREF(code);
  return (Object *)method;
}

Object *Method_Call(Object *self, Object *ob, Object *args)
{
  if (!Method_Check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }
  MethodObject *meth = (MethodObject *)self;
  if (meth->cfunc) {
    func_t fn = meth->ptr;
    return fn(ob, args);
  } else {
    return Koala_EvalCode(meth->ptr, ob, args);
  }
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
    int size = Tuple_Size(args);
    if (size <= 0)
      panic("args are less than 1");
    ob = Tuple_Get(args, 0);
    if (size == 2)
      para = Tuple_Get(args, 1);
    else
      para = Tuple_Slice(args, 1, -1);
  }

  MethodObject *meth = (MethodObject *)self;
  Object *res;
  if (meth->cfunc) {
    func_t fn = meth->ptr;
    res = fn(ob, para);
  } else {
    panic("not implemented");
    res = NULL;
  }
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

static Object *meth_str(Object *self, Object *args)
{
  if (!Method_Check(self)) {
    error("object of '%.64s' is not a Method", OB_TYPE_NAME(self));
    return NULL;
  }

  MethodObject *meth = (MethodObject *)self;
  return String_New(meth->name);
}

TypeObject method_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Method",
  .free    = meth_free,
  .str     = meth_str,
  .methods = meth_methods,
};

TypeObject proto_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Proto",
};

void init_method_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Method");
  method_type.desc = desc;
  if (type_ready(&method_type) < 0)
    panic("Cannot initalize 'Method' type.");
}

void init_proto_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Proto");
  proto_type.desc = desc;
  if (type_ready(&proto_type) < 0)
    panic("Cannot initalize 'Proto' type.");
}