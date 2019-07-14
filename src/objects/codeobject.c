/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/codeobject.h"

TypeObject code_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Code",
};

static Object *cfunc_new(TypeDesc *proto, cfunc_t func)
{
  CodeObject *code = kmalloc(sizeof(*code));
  init_object_head(code, &code_type);
  code->proto = TYPE_INCREF(proto);
  code->kind = CODE_CFUNC;
  code->cfunc = func;
  return (Object *)code;
}

Object *code_from_cfunc(struct cfuncdef *f)
{
  TypeDesc **para = typestr_toarr(f->ptype);
  TypeDesc **rets = typestr_toarr(f->rtype);
  TypeDesc *ret = rets ? rets[0] : NULL;
  kfree(rets);
  TypeDesc *proto = new_protodesc(para, ret);
  Object *code = cfunc_new(proto, f->func);
  TYPE_DECREF(ret);
  TYPE_DECREF(proto);
  return code;
}
