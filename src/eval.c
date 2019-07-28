/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "methodobject.h"
#include "log.h"

Object *Call_Function(Object *ob, char *name, Object *args)
{
  Object *func = Type_Get(OB_TYPE(ob), name);
  panic(Method_Check(func) < 0, "object of '%.64s' has no function '%.64s'.",
        OB_TYPE(ob)->name, name);
  MethodObject *method = (MethodObject *)func;
  if (method->cfunc) {
    cfunc fn = (cfunc)method->ptr;
    return fn(ob, args);
  } else {
    panic(1, "not implemented.");
    return NULL;
  }
}
