/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "methodobject.h"

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

TypeObject Method_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Method",
};

TypeObject Proto_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Proto",
};
