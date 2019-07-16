/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "objects/stringobject.h"
#include "objects/codeobject.h"
#include "atom.h"

MNode *new_mnode(int kind, char *name, TypeDesc *type)
{
  MNode *m = kmalloc(sizeof(*m));
  hashmap_entry_init(m, strhash(name));
  m->name = atom(name);
  m->kind = kind;
  m->desc = TYPE_INCREF(type);
  return m;
}

void free_mnode(MNode *m)
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

static int _mnode_cmp_cb_(void *e1, void *e2)
{
  if (e1 == e2)
    return 0;
  MNode *m1 = e1;
  MNode *m2 = e2;
  if (m1->name == m2->name)
    return 0;
  return strcmp(m1->name, m2->name);
}

static void _mnode_free_cb_(void *e, void *data)
{
  free_mnode(e);
}

void mtbl_init(MTable *mtbl)
{
  hashmap_init(&mtbl->tbl, _mnode_cmp_cb_);
}

void mtbl_fini(MTable *mtbl)
{
  hashmap_free(&mtbl->tbl, _mnode_free_cb_, NULL);
}

void mtbl_add_const(MTable *mtbl, char *name, TypeDesc *type, Object *val)
{

}

void mtbl_add_var(MTable *mtbl, char *name, TypeDesc *type)
{

}

void mtbl_add_func(MTable *mtbl, char *name, Object *code)
{
  struct codeobject *co = (struct codeobject *)code;
  MNode *m = new_mnode(MBR_FUNC, name, co->proto);
  m->code = OB_INCREF(code);
  hashmap_add(&mtbl->tbl, m);
}

void mtbl_add_cfuncs(MTable *mtbl, struct cfuncdef *funcs)
{
  Object *code;
  struct cfuncdef *f = funcs;
  while (f->name) {
    code = code_from_cfunc(f);
    mtbl_add_func(mtbl, f->name, code);
    OB_DECREF(code);
    f++;
  }
}

Klass class_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Class",
};

Klass any_type = {
  OBJECT_HEAD_INIT(&class_type)
  .name = "Any"
};

static Klass nil_type = {
  OBJECT_HEAD_INIT(&class_type)
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
  OB_TYPE_ASSERT(ob, &class_type);
  assert(!args);
  Klass *type = (Klass *)ob;
  return new_string(type->name);
}

static struct cfuncdef class_funcs[] = {
  {"__members__", NULL, "Llang.Tuple;", _class_members_},
  {"__name__", NULL, "s", _class_name_},
  {NULL}
};

void init_typeobject(void)
{
  mtbl_init(&class_type.mtbl);
  klass_add_cfuncs(&class_type, class_funcs);
}

void fini_typeobject(void)
{

}
