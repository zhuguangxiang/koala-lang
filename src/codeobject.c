/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "codeobject.h"

static void code_free(Object *ob)
{
  if (!Code_Check(ob)) {
    error("object of '%.64s' is not a Code", OB_TYPE_NAME(ob));
    return;
  }

  CodeObject *co = (CodeObject *)ob;
  VECTOR_ITERATOR(iter, &co->locvec);
  Object *item;
  iter_for_each(&iter, item) {
    OB_DECREF(item);
  }
  OB_DECREF(co->consts);
  TYPE_DECREF(co->proto);
  kfree(ob);
}

TypeObject Code_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Code",
  .free = code_free,
};

Object *Code_New(char *name, TypeDesc *proto, int locals,
                 uint8_t *codes, int size)
{
  CodeObject *co = kmalloc(sizeof(CodeObject) + size);
  Init_Object_Head(co, &Code_Type);
  co->name = name;
  co->proto = TYPE_INCREF(proto);
  vector_init(&co->locvec);
  co->locals = locals;
  co->size = size;
  memcpy(co->codes, codes, size);
  return (Object *)co;
}
