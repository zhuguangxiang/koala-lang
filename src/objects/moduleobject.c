/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "moduleobject.h"
#include "stringobject.h"
#include "fieldobject.h"
#include "classobject.h"
#include "tupleobject.h"
#include "node.h"
#include "log.h"

static VECTOR_PTR(modules);

static Object *module_get(Object *ob, char *name)
{
  if (Module_Check(ob) < 0) {
    error("object of '%.64s' is not a module.", OB_TYPE(ob)->name);
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)ob;
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  struct mnode *node = hashmap_get(module->mtbl, &key);
  return node ? node->obj : NULL;
}

static Object *module_name(Object *ob, Object *args)
{
  if (Module_Check(args) < 0) {
    error("object of '%.64s' is not a module.", OB_TYPE(ob)->name);
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)args;
  return String_New(module->name);
}

static Object *module_class(Object *ob, Object *args)
{
  if (!Field_Check(ob)) {
    error("object of '%.64s' is not a field.", OB_TYPE(ob)->name);
    return NULL;
  }

  if (Module_Check(args) < 0) {
    error("object of '%.64s' is not a module.", OB_TYPE(args)->name);
    return NULL;
  }

  ClassObject *cls = (ClassObject *)Class_New(args);
  cls->getfunc = module_get;
  return (Object *)cls;
}

TypeObject Module_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Module",
  .getfunc = module_get,
};

static struct hashmap *get_mtbl(ModuleObject *module)
{
  struct hashmap *mtbl = module->mtbl;
  if (mtbl == NULL) {
    mtbl = kmalloc(sizeof(*mtbl));
    panic(!mtbl, "memory allocation failed.");
    hashmap_init(mtbl, mnode_compare);
    module->mtbl = mtbl;
  }
  return mtbl;
}

int Module_Add_Const(Object *ob, Object *val)
{
  FieldObject *field = (FieldObject *)val;
  struct mnode *node = mnode_new(field->name, val);
  field->owner = ob;
  return hashmap_add(get_mtbl((ModuleObject *)ob), node);
}

Object *Module_New(char *name)
{
  ModuleObject *module = kmalloc(sizeof(*module));
  Init_Object_Head(module, &Module_Type);
  module->name = name;
  Object *ob = (Object *)module;

  Object *field = Field_New("__name__", "s", module_name, NULL);
  Module_Add_Const(ob, field);

  field = Field_New("__class__", "Llang.Class;", module_class, NULL);
  Module_Add_Const(ob, field);

  return ob;
}

void Module_Install(char *path, Object *ob)
{
  if (Module_Check(ob) < 0) {
    error("object of '%.64s' is not a module.", OB_TYPE(ob)->name);
    return;
  }

  ModuleObject *module = (ModuleObject *)ob;
  char **name = path_toarr(path, strlen(path));
  int res = add_leaf(name, ob);
  if (!res) {
    vector_push_back(&modules, ob);
    debug("install module '%.64s' in path '%s' successfully.",
          module->name, path);
  } else {
    error("install module '%.64s' in path '%s' failed.",
          module->name, path);
  }
  kfree(name);
}
