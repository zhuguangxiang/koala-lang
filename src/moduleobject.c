/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "moduleobject.h"
#include "stringobject.h"
#include "methodobject.h"
#include "fieldobject.h"
#include "classobject.h"
#include "tupleobject.h"
#include "hashmap.h"
#include "log.h"

Object *Module_Lookup(Object *ob, char *name)
{
  if (!Module_Check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)ob;
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  struct mnode *node = hashmap_get(module->mtbl, &key);
  if (node != NULL)
    return OB_INCREF(node->obj);

  return Type_Lookup(OB_TYPE(ob), name);
}

static Object *module_name(Object *self, Object *args)
{
  if (!Module_Check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)self;
  return String_New(module->name);
}

static MethodDef module_methods[] = {
  {"__name__", NULL, "s", module_name},
  {NULL}
};

TypeObject Module_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Module",
  .methods = module_methods,
};

static HashMap *get_mtbl(Object *ob)
{
  ModuleObject *module = (ModuleObject *)ob;
  HashMap *mtbl = module->mtbl;
  if (mtbl == NULL) {
    mtbl = kmalloc(sizeof(*mtbl));
    if (!mtbl)
      panic("memory allocation failed.");
    hashmap_init(mtbl, mnode_equal);
    module->mtbl = mtbl;
  }
  return mtbl;
}

#define MODULE_NAME(ob) (((ModuleObject *)ob)->name)

void Module_Add_Type(Object *self, TypeObject *type)
{
  type->owner = self;
  struct mnode *node = mnode_new(type->name, (Object *)type);
  int res = hashmap_add(get_mtbl(self), node);
  if (res != 0)
    panic("'%.64s' add '%.64s' failed.", MODULE_NAME(self), type->name);
}

void Module_Add_Const(Object *self, Object *ob, Object *val)
{
  FieldObject *field = (FieldObject *)ob;
  field->owner = self;
  field->value = OB_INCREF(val);
  struct mnode *node = mnode_new(field->name, ob);
  int res = hashmap_add(get_mtbl(self), node);
  if (res != 0)
    panic("'%.64s' add '%.64s' failed.", MODULE_NAME(self), field->name);
}

void Module_Add_Var(Object *self, Object *ob)
{
  FieldObject *field = (FieldObject *)ob;
  struct mnode *node = mnode_new(field->name, ob);
  field->owner = self;
  int res = hashmap_add(get_mtbl(self), node);
  if (res != 0)
    panic("'%.64s' add '%.64s' failed.", MODULE_NAME(self), field->name);
}

void Module_Add_VarDef(Object *self, FieldDef *f)
{
  Object *field = Field_New(f);
  Module_Add_Var(self, field);
}

void Module_Add_VarDefs(Object *self, FieldDef *def)
{
  FieldDef *f = def;
  while (f->name) {
    Module_Add_VarDef(self, f);
    ++f;
  }
}

void Module_Add_Func(Object *self, Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  struct mnode *node = mnode_new(meth->name, (Object *)meth);
  meth->owner = self;
  int res = hashmap_add(get_mtbl(self), node);
  if (res != 0)
    panic("'%.64s' add '%.64s' failed.", MODULE_NAME(self), meth->name);
}

void Module_Add_FuncDef(Object *self, MethodDef *f)
{
  Object *meth = CMethod_New(f);
  Module_Add_Func(self, meth);
}

void Module_Add_FuncDefs(Object *self, MethodDef *def)
{
  MethodDef *f = def;
  while (f->name) {
    Module_Add_FuncDef(self, f);
    ++f;
  }
}

Object *Module_New(char *name)
{
  ModuleObject *module = kmalloc(sizeof(*module));
  Init_Object_Head(module, &Module_Type);
  module->name = name;
  Object *ob = (Object *)module;
  return ob;
}

struct modnode {
  HashMapEntry entry;
  char *path;
  Object *ob;
};

static HashMap modmap;

static int _modnode_equal_(void *e1, void *e2)
{
  struct modnode *n1 = e1;
  struct modnode *n2 = e2;
  return n1 == n2 || !strcmp(n1->path, n2->path);
}

void Module_Install(char *path, Object *ob)
{
  if (Module_Check(ob) < 0) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return;
  }

  if (!modmap.entries) {
    hashmap_init(&modmap, _modnode_equal_);
  }

  struct modnode *node = kmalloc(sizeof(*node));
  hashmap_entry_init(node, strhash(path));
  node->path = path;
  node->ob = ob;
  int res = hashmap_add(&modmap, node);
  if (!res) {
    debug("install module '%.64s' in path '%.64s' successfully.",
          MODULE_NAME(ob), path);
  } else {
    error("install module '%.64s' in path '%.64s' failed.",
          MODULE_NAME(ob), path);
    kfree(node);
  }
}

Object *Module_Load(char *path)
{
  struct modnode key = {.path = path};
  hashmap_entry_init(&key, strhash(path));
  struct modnode *node = hashmap_get(&modmap, &key);
  if (node == NULL) {
    /* find its source and compile it */
    panic("cannot find module '%.64s'", path);
  }
  return OB_INCREF(node->ob);
}

void Module_UnLoad(char *path)
{
  struct modnode key = {.path = path};
  hashmap_entry_init(&key, strhash(path));
  struct modnode *node = hashmap_remove(&modmap, &key);
  Object *ob = NULL;
  if (node != NULL) {
    ob = node->ob;
    kfree(node);
  }
}
