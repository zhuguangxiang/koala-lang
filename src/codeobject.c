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
  Object *item;
  vector_for_each(item, &co->locvec) {
    OB_DECREF(item);
  }
  OB_DECREF(co->consts);
  TYPE_DECREF(co->proto);
  kfree(ob);
}

TypeObject code_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Code",
  .free = code_free,
};

Object *Code_New(char *name, TypeDesc *proto, int locals,
                 uint8_t *codes, int size)
{
  CodeObject *co = kmalloc(sizeof(CodeObject) + size);
  init_object_head(co, &code_type);
  co->name = name;
  co->proto = TYPE_INCREF(proto);
  vector_init(&co->locvec);
  co->locals = locals;
  co->size = size;
  memcpy(co->codes, codes, size);
  return (Object *)co;
}
