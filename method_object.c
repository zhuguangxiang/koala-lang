
#include "method_object.h"
#include "module_object.h"

static struct method_object *new_method(int extra_size)
{
  struct method_object *fo = malloc(sizeof(*fo) + extra_size);
  init_object_head(fo, &method_klass);
  assert(fo);
  return fo;
}

void free_method(struct object *ob)
{
  assert(OB_KLASS(ob) == &method_klass);
}

struct object *new_cfunc(cfunc_t func)
{
  struct method_object *fo = new_method(sizeof(cfunc_t));
  fo->flags = METH_CFUNC;
  *((cfunc_t *)(fo + 1)) = func;
  return (struct object *)fo;
}

struct object *new_kfunc(void *codes, void *consts)
{
  struct method_object *fo = new_method(sizeof(struct code_struct));
  fo->flags = METH_KFUNC;
  struct code_struct *cs = (struct code_struct *)(fo + 1);
  cs->codes = codes;
  cs->linkages = consts;
  return (struct object *)fo;
}

struct object *new_closure(void *codes, char *consts)
{
  return NULL;
}

struct klass_object method_klass = {
  OBJECT_HEAD_INIT(&klass_klass),
  .name  = "Method",
  .bsize = sizeof(struct method_object),
};

void init_method_klass(void)
{
  struct object *ko = (struct object *)&method_klass;

  struct object *mo = find_module("koala/lang");
  assert(mo);

  int res = module_add(mo, new_klass_namei("Method", 0), ko);
  assert(!res);
}
