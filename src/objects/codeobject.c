/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/codeobject.h"

struct klass code_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Code",
};

void init_code_type(void)
{

}

void free_code_type(void)
{

}

static struct object *cfunc_new(struct typedesc *proto, cfunc_t func)
{
  struct code_object *code = kmalloc(sizeof(*code));
  init_object_head(code, &code_type);
  code->proto = TYPE_INCREF(proto);
  code->kind = CODE_CFUNC;
  code->cfunc = func;
  return (struct object *)code;
}

struct object *code_from_cfunc(struct cfuncdef *f)
{
  struct typedesc **para = typestr_toarr(f->ptype);
  struct typedesc **rets = typestr_toarr(f->rtype);
  struct typedesc *ret = rets ? rets[0] : NULL;
  kfree(rets);
  struct typedesc *proto = new_protodesc(para, ret);
  struct object *code = cfunc_new(proto, f->func);
  TYPE_DECREF(ret);
  TYPE_DECREF(proto);
  return code;
}
