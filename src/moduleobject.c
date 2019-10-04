/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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
  return string_new(module->name);
}

static void module_free(Object *ob)
{
  if (!Module_Check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return;
  }

  ModuleObject *module = (ModuleObject *)ob;
  HashMap *map = module->mtbl;
  if (map != NULL) {
    debug("fini module '%s'", module->name);
    hashmap_fini(map, mnode_free, NULL);
    debug("------------------------");
    kfree(map);
    module->mtbl = NULL;
  }

  Object *item;
  vector_for_each(item, &module->values) {
    OB_DECREF(item);
  }
  vector_fini(&module->values);
  kfree(ob);
}

static MethodDef module_methods[] = {
  {"__name__", NULL, "s", module_name},
  {NULL}
};

TypeObject module_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Module",
  .free    = module_free,
  .methods = module_methods,
};

void init_module_type(void)
{
  TypeDesc *desc = desc_from_klass("lang", "Module");
  module_type.desc = desc;
  if (type_ready(&module_type) < 0)
    panic("Cannot initalize 'Module' type.");
}

static HashMap *get_mtbl(Object *ob)
{
  ModuleObject *module = (ModuleObject *)ob;
  HashMap *mtbl = module->mtbl;
  if (mtbl == NULL) {
    mtbl = kmalloc(sizeof(*mtbl));
    hashmap_init(mtbl, mnode_equal);
    module->mtbl = mtbl;
  }
  return mtbl;
}

void Module_Add_Type(Object *self, TypeObject *type)
{
  type->owner = self;
  struct mnode *node = mnode_new(type->name, (Object *)type);
  int res = hashmap_add(get_mtbl(self), node);
  expect(res == 0);
}

void Module_Add_Const(Object *self, Object *ob, Object *val)
{
  ModuleObject *module = (ModuleObject *)self;
  FieldObject *field = (FieldObject *)ob;
  field->owner = self;
  field->offset = module->nrvars;
  vector_set(&module->values, module->nrvars, OB_INCREF(val));
  ++module->nrvars;
  struct mnode *node = mnode_new(field->name, ob);
  int res = hashmap_add(get_mtbl(self), node);
  expect(res == 0);
}

void Module_Add_Var(Object *self, Object *ob)
{
  ModuleObject *module = (ModuleObject *)self;
  FieldObject *field = (FieldObject *)ob;
  field->owner = self;
  field->offset = module->nrvars;
  ++module->nrvars;
  // occurpy a place holder
  vector_set(&module->values, field->offset, NULL);
  struct mnode *node = mnode_new(field->name, ob);
  int res = hashmap_add(get_mtbl(self), node);
  expect(res == 0);
}

void Module_Add_VarDef(Object *self, FieldDef *f)
{
  TypeDesc *desc = str_to_desc(f->type);
  Object *field = field_new(f->name, desc);
  TYPE_DECREF(desc);
  Field_SetFunc(field, f->set, f->get);
  Module_Add_Var(self, field);
  OB_DECREF(field);
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
  struct mnode *node = mnode_new(meth->name, ob);
  meth->owner = self;
  int res = hashmap_add(get_mtbl(self), node);
  expect(res == 0);
}

void Module_Add_FuncDef(Object *self, MethodDef *f)
{
  Object *meth = CMethod_New(f);
  Module_Add_Func(self, meth);
  OB_DECREF(meth);
}

void Module_Add_FuncDefs(Object *self, MethodDef *def)
{
  MethodDef *f = def;
  while (f->name) {
    Module_Add_FuncDef(self, f);
    ++f;
  }
}

Object *module_get(Object *self, char *name)
{
  Object *ob = Module_Lookup(self, name);
  expect(ob != NULL);

  if (field_check(ob)) {
    Object *res = field_get(ob, self);
    OB_DECREF(ob);
    return res;
  }

  expect(method_check(ob));
  return ob;
}

void module_set(Object *self, char *name, Object *val)
{
  Object *ob = Module_Lookup(self, name);
  expect(ob != NULL);
  expect(field_check(ob));
  int res = field_set(ob, self, val);
  expect(res == 0);
}

Object *Module_New(char *name)
{
  ModuleObject *module = kmalloc(sizeof(*module));
  init_object_head(module, &module_type);
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
    debug("init module map");
    hashmap_init(&modmap, _modnode_equal_);
  }

  ModuleObject *mob = (ModuleObject *)ob;
  mob->path = path;
  struct modnode *node = kmalloc(sizeof(*node));
  hashmap_entry_init(node, strhash(path));
  node->path = path;
  node->ob = OB_INCREF(ob);
  int res = hashmap_add(&modmap, node);
  if (!res) {
    debug("install module '%.64s' in path '%.64s' successfully.",
          MODULE_NAME(ob), path);
  } else {
    error("install module '%.64s' in path '%.64s' failed.",
          MODULE_NAME(ob), path);
    mnode_free(node, NULL);
  }
}

void Module_Uninstall(char *path)
{
  struct modnode key = {.path = path};
  hashmap_entry_init(&key, strhash(path));
  struct modnode *node = hashmap_remove(&modmap, &key);
  Object *ob = NULL;
  if (node != NULL) {
    mnode_free(node, NULL);
  }

  if (hashmap_size(&modmap) <= 0) {
    debug("fini module map");
    hashmap_fini(&modmap, NULL, NULL);
  }
}

Object *Module_Load(char *path)
{
  struct modnode key = {.path = path};
  hashmap_entry_init(&key, strhash(path));
  struct modnode *node = hashmap_get(&modmap, &key);
  /* find its source and compile it */
  expect(node != NULL);
  return OB_INCREF(node->ob);
}
