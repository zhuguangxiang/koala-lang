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

#include <dlfcn.h>
#include "moduleobject.h"
#include "stringobject.h"
#include "methodobject.h"
#include "fieldobject.h"
#include "classobject.h"
#include "tupleobject.h"
#include "codeobject.h"
#include "hashmap.h"
#include "image.h"
#include "log.h"

Object *module_lookup(Object *ob, char *name)
{
  if (!module_check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)ob;
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  struct mnode *node = hashmap_get(module->mtbl, &key);
  if (node != NULL)
    return OB_INCREF(node->obj);

  return type_lookup(OB_TYPE(ob), name);
}

static Object *module_name(Object *self, Object *args)
{
  if (!module_check(self)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(self));
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)self;
  return string_new(module->name);
}

static void module_free(Object *ob)
{
  if (!module_check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return;
  }

  ModuleObject *module = (ModuleObject *)ob;
  debug("[Freed] Module '%s'", module->name);

  Object *item;
  vector_for_each(item, &module->values) {
    OB_DECREF(item);
  }
  vector_fini(&module->values);

  OB_DECREF(module->consts);

  HashMap *map = module->mtbl;
  if (map != NULL) {
    hashmap_fini(map, mnode_free, NULL);
    debug("------------------------");
    kfree(map);
    module->mtbl = NULL;
  }

  if (module->dlptr != NULL) {
    dlclose(module->dlptr);
  }

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

void module_add_type(Object *self, TypeObject *type)
{
  type->owner = self;
  struct mnode *node = mnode_new(type->name, (Object *)type);
  int res = hashmap_add(get_mtbl(self), node);
  expect(res == 0);
}

void module_add_var(Object *self, Object *ob)
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

void module_add_vardef(Object *self, FieldDef *f)
{
  TypeDesc *desc = str_to_desc(f->type);
  Object *field = field_new(f->name, desc);
  TYPE_DECREF(desc);
  Field_SetFunc(field, f->set, f->get);
  module_add_var(self, field);
  OB_DECREF(field);
}

void module_add_vardefs(Object *self, FieldDef *def)
{
  FieldDef *f = def;
  while (f->name) {
    module_add_vardef(self, f);
    ++f;
  }
}

void module_add_func(Object *self, Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  struct mnode *node = mnode_new(meth->name, ob);
  meth->owner = self;
  int res = hashmap_add(get_mtbl(self), node);
  expect(res == 0);
}

void module_add_funcdef(Object *self, MethodDef *f)
{
  Object *meth = CMethod_New(f);
  module_add_func(self, meth);
  OB_DECREF(meth);
}

void module_add_funcdefs(Object *self, MethodDef *def)
{
  MethodDef *f = def;
  while (f->name) {
    module_add_funcdef(self, f);
    ++f;
  }
}

Object *module_get(Object *self, char *name)
{
  Object *ob = module_lookup(self, name);
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
  Object *ob = module_lookup(self, name);
  expect(ob != NULL);
  expect(field_check(ob));
  int res = field_set(ob, self, val);
  expect(res == 0);
}

Object *module_new(char *name)
{
  ModuleObject *module = kmalloc(sizeof(*module));
  init_object_head(module, &module_type);
  module->name = name;
  return (Object *)module;
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

void module_install(char *path, Object *ob)
{
  if (module_check(ob) < 0) {
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

void module_uninstall(char *path)
{
  struct modnode key = {.path = path};
  hashmap_entry_init(&key, strhash(path));
  struct modnode *node = hashmap_remove(&modmap, &key);
  if (node != NULL) {
    OB_DECREF(node->ob);
    kfree(node);
  }

  if (hashmap_size(&modmap) <= 0) {
    debug("fini module map");
    hashmap_fini(&modmap, NULL, NULL);
  }
}

static void _fini_mod_(void *e, void *arg)
{
  struct modnode *node = e;
  OB_DECREF(node->ob);
  kfree(node);
}

void fini_modules(void)
{
  debug("fini module map");
  hashmap_fini(&modmap, _fini_mod_, NULL);
}

void _load_const_(void *val, int kind, int index, void *arg)
{
  Object *mo = arg;
  Object *consts = ((ModuleObject *)mo)->consts;
  Object *ob;
  CodeInfo *ci;

  if (kind == CONST_LITERAL) {
    ob = new_literal(val);
  } else if (kind == CONST_TYPE) {
    ob = new_descob(val);
  } else {
    expect(kind == CONST_ANONY);
    ci = val;
    ob = code_new(ci->name, ci->desc, ci->codes, ci->size);
    code_set_module(ob, mo);
    code_set_consts(ob, consts);
    code_set_locvars(ob, ci->locvec);
    code_set_freevals(ob, ci->freevec);
    code_set_upvals(ob, ci->upvec);
    debug("'%s': %d locvars, %d freevals and %d upvals",
          ci->name, vector_size(ci->locvec),
          vector_size(ci->freevec), vector_size(ci->upvec));
  }

  tuple_set(consts, index, ob);
  OB_DECREF(ob);
}

static void _load_var_(char *name, int kind, TypeDesc *desc, void *arg)
{
  Object *field = field_new(name, desc);
  Field_SetFunc(field, field_default_setter, field_default_getter);
  module_add_var(arg, field);
  OB_DECREF(field);
}

static void _load_func_(char *name, CodeInfo *ci, void *arg)
{
  ModuleObject *module = (ModuleObject *)arg;
  Object *code = code_new(name, ci->desc, ci->codes, ci->size);
  code_set_locvars(code, ci->locvec);
  code_set_freevals(code, ci->freevec);
  code_set_module(code, arg);
  code_set_consts(code, module->consts);
  Object *meth = Method_New(name, code);
  module_add_func(arg, meth);
  OB_DECREF(code);
  OB_DECREF(meth);
}

static Object *module_from_file(char *path, char *name)
{
  char *klcpath = str_dup_ex(path, ".klc");
  debug("read image from '%s'", klcpath);
  Image *image = image_read_file(klcpath, 0);
  Object *mo = NULL;
  if (image != NULL) {
    mo = module_new(name);
    Object *consts;
    int size = _size_(image, ITEM_CONST);
    if (size > 0)
      consts = tuple_new(size);
    else
      consts = NULL;
    ((ModuleObject *)mo)->consts = consts;
    image_load_consts(image, _load_const_, mo);
    IMAGE_LOAD_VARS(image, _load_var_, mo);
    IMAGE_LOAD_CONSTVARS(image, _load_var_, mo);
    IMAGE_LOAD_FUNCS(image, _load_func_, mo);
    /*
    IMAGE_LOAD_CLASSES(image, _load_class_, mo);
    IMAGE_LOAD_TRAITS(image, _load_trait_, mo);
    IMAGE_LOAD_ENUMS(image, _load_enum_, mo);
    */
    image_free(image);
  } else {
    warn("cannot load '%s'", klcpath);
  }
  kfree(klcpath);
  return mo;
}

// path: a/b/foo, not allowed a/b/foo/
Object *module_load(char *path)
{
  struct modnode key = {.path = path};
  hashmap_entry_init(&key, strhash(path));
  struct modnode *node = hashmap_get(&modmap, &key);
  if (node == NULL) {
    char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    Object *mo = module_from_file(path, name);
    if (mo == NULL) {
      warn("cannot find module '%s'", path);
      return NULL;
    } else {
      module_install(path, mo);
      return mo;
    }
  }
  return OB_INCREF(node->ob);
}
