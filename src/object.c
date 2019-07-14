/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "string_object.h"
#include "code_object.h"
#include "atom.h"

struct member *new_member(int kind, char *name, struct typedesc *type)
{
  struct member *m = kmalloc(sizeof(*m));
  hashmap_entry_init(m, strhash(name));
  m->name = atom(name);
  m->kind = kind;
  m->type = TYPE_INCREF(type);
  return m;
}

void free_member(struct member *m)
{
  switch (m->kind) {
  case MBR_FUNC: {
    OB_DECREF(m->code);
    break;
  }
  default:{
    assert(0);
    break;
  }
  }
  TYPE_DECREF(m->type);
  kfree(m);
}

static int __member_cmp_cb__(void *e1, void *e2)
{
  if (e1 == e2)
    return 0;
  struct member *m1 = e1;
  struct member *m2 = e2;
  if (m1->name == m2->name)
    return 0;
  return strcmp(m1->name, m2->name);
}

static void __member_free_cb__(void *e, void *data)
{
  free_member(e);
}

void mtable_init(struct mtable *mtbl)
{
  hashmap_init(&mtbl->tbl, __member_cmp_cb__);
}

void mtable_free(struct mtable *mtbl)
{
  hashmap_free(&mtbl->tbl, __member_free_cb__, NULL);
}

void mtable_add_const(struct mtable *mtbl, char *name,
                      struct typedesc *type, struct object *val)
{

}

void mtable_add_var(struct mtable *mtbl, char *name, struct typedesc *type)
{

}

void mtable_add_func(struct mtable *mtbl, char *name, struct object *code)
{
  struct code_object *co = (struct code_object *)code;
  struct member *m = new_member(MBR_FUNC, name, co->proto);
  m->code = OB_INCREF(code);
  hashmap_add(&mtbl->tbl, m);
}

void mtable_add_cfuncs(struct mtable *mtbl, struct cfuncdef *funcs)
{
  struct object *code;
  struct cfuncdef *f = funcs;
  while (f->name) {
    code = code_from_cfunc(f);
    mtable_add_func(mtbl, f->name, code);
    OB_DECREF(code);
    f++;
  }
}

struct klass class_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Class",
};

struct klass any_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Any"
};

struct klass nil_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "NilType"
};

struct object nil_obj = {
  OBJECT_HEAD_INIT(&nil_type)
};

struct object *__class_members__(struct object *ob, struct object *args)
{
  return NULL;
}

struct object *__class_name__(struct object *ob, struct object *args)
{
  OB_TYPE_ASSERT(ob, &class_type);
  assert(!args);
  struct klass *type = (struct klass *)ob;
  return new_string(type->name);
}

static struct cfuncdef class_funcs[] = {
  {"__members__", NULL, "Llang.Tuple;", __class_members__},
  {"__name__", NULL, "s", __class_name__},
  {NULL}
};

void init_klass_type(void)
{
  mtable_init(&class_type.mtbl);
  klass_add_cfuncs(&class_type, class_funcs);
}

void free_klass_type(void)
{

}
