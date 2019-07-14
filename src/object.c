/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/stringobject.h"
#include "objects/codeobject.h"
#include "atom.h"

struct member *new_member(int kind, char *name, TypeDesc *type)
{
  struct member *m = kmalloc(sizeof(*m));
  hashmap_entry_init(m, strhash(name));
  m->name = atom(name);
  m->kind = kind;
  m->desc = TYPE_INCREF(type);
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
  TYPE_DECREF(m->desc);
  kfree(m);
}

static int _member_cmp_cb_(void *e1, void *e2)
{
  if (e1 == e2)
    return 0;
  struct member *m1 = e1;
  struct member *m2 = e2;
  if (m1->name == m2->name)
    return 0;
  return strcmp(m1->name, m2->name);
}

static void _member_free_cb_(void *e, void *data)
{
  free_member(e);
}

void mtable_init(struct mtable *mtbl)
{
  hashmap_init(&mtbl->tbl, _member_cmp_cb_);
}

void mtable_fini(struct mtable *mtbl)
{
  hashmap_free(&mtbl->tbl, _member_free_cb_, NULL);
}

void mtable_add_const(struct mtable *mtbl, char *name,
                      TypeDesc *type, Object *val)
{

}

void mtable_add_var(struct mtable *mtbl, char *name, TypeDesc *type)
{

}

void mtable_add_func(struct mtable *mtbl, char *name, Object *code)
{
  struct codeobject *co = (struct codeobject *)code;
  struct member *m = new_member(MBR_FUNC, name, co->proto);
  m->code = OB_INCREF(code);
  hashmap_add(&mtbl->tbl, m);
}

void mtable_add_cfuncs(struct mtable *mtbl, struct cfuncdef *funcs)
{
  Object *code;
  struct cfuncdef *f = funcs;
  while (f->name) {
    code = code_from_cfunc(f);
    mtable_add_func(mtbl, f->name, code);
    OB_DECREF(code);
    f++;
  }
}

TypeObject type_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "TypeType",
};

TypeObject any_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Any"
};

static TypeObject nil_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "NilType"
};

Object nil_object = {
  OBJECT_HEAD_INIT(&nil_type)
};

Object *_class_members_(Object *ob, Object *args)
{
  return NULL;
}

Object *_class_name_(Object *ob, Object *args)
{
  OB_TYPE_ASSERT(ob, &type_type);
  assert(!args);
  TypeObject *type = (TypeObject *)ob;
  return new_string(type->name);
}

static struct cfuncdef class_funcs[] = {
  {"__members__", NULL, "Llang.Tuple;", _class_members_},
  {"__name__", NULL, "s", _class_name_},
  {NULL}
};

void init_typeobject(void)
{
  mtable_init(&type_type.mtbl);
  klass_add_cfuncs(&type_type, class_funcs);
}

void fini_typeobject(void)
{

}
