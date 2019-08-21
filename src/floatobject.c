/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "floatobject.h"
#include "stringobject.h"

static void float_free(Object *ob)
{
  if (!Float_Check(ob)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(ob));
    return;
  }
  FloatObject *f = (FloatObject *)ob;
  debug("[Freed] float %lf", f->value);
  kfree(ob);
}

static Object *float_str(Object *self, Object *ob)
{
  if (!Float_Check(self)) {
    error("object of '%.64s' is not a Float", OB_TYPE_NAME(self));
    return NULL;
  }

  FloatObject *f = (FloatObject *)self;
  char buf[256];
  sprintf(buf, "%lf", f->value);
  return String_New(buf);
}

TypeObject Float_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Float",
  .str  = float_str,
  .free = float_free,
};

void init_floatobject(void)
{
}

void fini_floatobject(void)
{
}

Object *Float_New(double val)
{
  FloatObject *f = kmalloc(sizeof(FloatObject));
  Init_Object_Head(f, &Float_Type);
  f->value = val;
  return (Object *)f;
}
