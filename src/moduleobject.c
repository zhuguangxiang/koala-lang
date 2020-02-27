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
#include "classobject.h"
#include "tupleobject.h"
#include "codeobject.h"
#include "enumobject.h"
#include "hashmap.h"
#include "image.h"
#include "log.h"
#include "atom.h"

Object *module_lookup(Object *ob, char *name)
{
  if (!module_check(ob)) {
    error("object of '%.64s' is not a Module", OB_TYPE_NAME(ob));
    return NULL;
  }

  ModuleObject *module = (ModuleObject *)ob;
  // find from cache
  struct mnode *node;
  vector_for_each(node, &module->cache) {
    if (!strcmp(node->name, name)) {
      debug("find '%s' from module '%s' cache", name, module->name);
      return OB_INCREF(node->obj);
    }
  }

  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  node = hashmap_get(module->mtbl, &key);
  if (node != NULL) {
    if (vector_size(&module->cache) >= 5) {
      // cache latest 5 objects
      vector_pop_back(&module->cache);
    }
    vector_push_back(&module->cache, node);
    return OB_INCREF(node->obj);
  }

  return type_lookup(OB_TYPE(ob), NULL, name);
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

static void nfnode_free(void *e, void *arg);

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

  vector_fini(&module->cache);

  map = module->nftbl;
  if (map != NULL) {
    hashmap_fini(map, nfnode_free, NULL);
    debug("------------------------");
    kfree(map);
    module->nftbl = NULL;
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

void module_add_var(Object *self, Object *ob, Object *defval)
{
  ModuleObject *module = (ModuleObject *)self;
  FieldObject *field = (FieldObject *)ob;
  field->owner = self;
  field->offset = module->nrvars++;
  // set var's default value
  vector_set(&module->values, field->offset, defval);
  OB_INCREF(defval);
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
  Object *defval = (f->defval != NULL) ? f->defval() : NULL;
  module_add_var(self, field, defval);
  OB_DECREF(defval);
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
  Object *meth = cmethod_new(f);
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

struct nfnode {
  HashMapEntry entry;
  char *name;
  void *code;
};

static int nfnode_equal(void *e1, void *e2)
{
  struct nfnode *n1 = e1;
  struct nfnode *n2 = e2;
  return (n1 == n2) || !strcmp(n1->name, n2->name);
}

static struct nfnode *nfnode_new(char *name, void *code)
{
  struct nfnode *node = kmalloc(sizeof(*node));
  node->name = name;
  hashmap_entry_init(node, strhash(name));
  node->code = code;
  return node;
}

static void nfnode_free(void *e, void *arg)
{
  struct nfnode *node = e;
  debug("[NFuncNode Freed] '%s'", node->name);
  kfree(node);
}

static HashMap *get_nftbl(ModuleObject *mob)
{
  HashMap *nftbl = mob->nftbl;
  if (nftbl == NULL) {
    nftbl = kmalloc(sizeof(*nftbl));
    hashmap_init(nftbl, nfnode_equal);
    mob->nftbl = nftbl;
  }
  return nftbl;
}

void *module_get_native(Object *self, char *name)
{
  ModuleObject *module = (ModuleObject *)self;
  struct nfnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));
  struct nfnode *node = hashmap_get(get_nftbl(module), &key);
  if (node != NULL) {
    debug("find native function '%s'", name);
    return node->code;
  } else {
    warn("cannot find native function '%s'", name);
    return NULL;
  }
}

void module_add_native(Object *self, char *name, void *code)
{
  ModuleObject *module = (ModuleObject *)self;
  struct nfnode *node = nfnode_new(name, code);
  int res = hashmap_add(get_nftbl(module), node);
  expect(res == 0);
}

Object *module_new(char *name)
{
  ModuleObject *module = kmalloc(sizeof(*module));
  init_object_head(module, &module_type);
  module->name = name;
  module->ready = 1;
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

extern int compflag;
extern void run_func(Object *mo, char *funcname, Object *args);

void module_install(char *path, Object *ob)
{
  if (!module_check(ob)) {
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
    if (!compflag) {
      run_func(ob, "__init__", NULL);
    }
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
  debug("fini module '%s'", node->path);
  OB_DECREF(node->ob);
  kfree(node);
}

void fini_modules(void)
{
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
  module_add_var(arg, field, NULL);
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
  Object *meth = method_new(name, code);
  module_add_func(arg, meth);
  OB_DECREF(code);
  OB_DECREF(meth);
}

static void _load_nfunc_(char *name, int kind, void *data, void *arg)
{
  expect(kind == MBR_IFUNC);
  void *code = module_get_native(arg, name);
  Object *meth = nmethod_new(name, data, code);
  module_add_func(arg, meth);
  OB_DECREF(meth);
}

static void _load_mbr_(char *name, int kind, void *data, void *arg)
{
  TypeObject *type = arg;
  if (kind == MBR_FIELD) {
    type_add_field_default(type, atom(name), data);
  } else if (kind == MBR_METHOD) {
    CodeInfo *ci = data;
    Object *code = code_new(name, ci->desc, ci->codes, ci->size);
    code_set_locvars(code, ci->locvec);
    code_set_freevals(code, ci->freevec);
    code_set_module(code, type->owner);
    code_set_type(code, type);
    code_set_consts(code, type->consts);
    Object *meth = method_new(name, code);
    type_add_method(type, meth);
    OB_DECREF(code);
    OB_DECREF(meth);
  } else if (kind == MBR_LABEL) {
    type_add_label(type, name, data);
  } else if (kind == MBR_IFUNC) {
    Object *proto = proto_new(name, data);
    type_add_ifunc(type, proto);
    OB_DECREF(proto);
  } else {
    panic("_load_mbr__: not implemented");
  }
}

static void _load_base_(TypeDesc *desc, void *arg)
{
  TypeObject *type = arg;
  expect(desc->kind == TYPE_KLASS);

  Object *tp;
  if (desc->klass.path != NULL) {
    Object *m = module_load(desc->klass.path);
    tp = module_get(m, desc->klass.type);
    OB_DECREF(m);
  } else {
    tp = module_get(type->owner, desc->klass.type);
  }

  expect(tp != NULL);
  expect(type_check(tp));
  type_add_base(type, tp);
  OB_DECREF(tp);
}

static
void _load_class_(char *name, int baseidx, int mbridx, Image *image, void *arg)
{
  ModuleObject *mo = (ModuleObject *)arg;
  TypeObject *type = type_new(mo->path, name, TPFLAGS_CLASS);
  module_add_type(arg, type);
  type->consts = mo->consts;
  OB_INCREF(type->consts);
  OB_DECREF(type);

  image_load_mbrs(image, mbridx, _load_mbr_, type);
  if (baseidx >= 0)
    image_load_bases(image, baseidx, _load_base_, type);

  if (type_ready(type) < 0)
    panic("Cannot initalize '%s' type.", name);
}

static
void _load_trait_(char *name, int baseidx, int mbridx, Image *image, void *arg)
{
  ModuleObject *mo = (ModuleObject *)arg;
  TypeObject *type = type_new(mo->path, name, TPFLAGS_TRAIT);
  module_add_type(arg, type);
  type->consts = mo->consts;
  OB_INCREF(type->consts);
  OB_DECREF(type);

  image_load_mbrs(image, mbridx, _load_mbr_, type);
  if (baseidx >= 0)
    image_load_bases(image, baseidx, _load_base_, type);

  if (type_ready(type) < 0)
    panic("Cannot initalize '%s' type.", name);
}

static
void _load_enum_(char *name, int baseidx, int mbridx, Image *image, void *arg)
{
  ModuleObject *mo = (ModuleObject *)arg;
  TypeObject *type = enum_type_new(mo->path, name);
  module_add_type(arg, type);
  type->consts = mo->consts;
  OB_INCREF(type->consts);
  OB_DECREF(type);

  image_load_mbrs(image, mbridx, _load_mbr_, type);
  expect(baseidx < 0);

  if (type_ready(type) < 0)
    panic("Cannot initalize '%s' type.", name);
}

static Object *module_from_file(char *path, char *name, Object *ob)
{
  char *klcpath = str_dup_ex(path, ".klc");
  debug("read image from '%s'", klcpath);
  Image *image = image_read_file(klcpath, 0);
  Object *mo = ob;
  if (image != NULL) {
    if (mo == NULL)
      mo = module_new(name);
    Object *consts;
    int size = _size_(image, ITEM_CONST);
    if (size > 0)
      consts = tuple_new(size);
    else
      consts = NULL;
    ((ModuleObject *)mo)->consts = consts;
    ((ModuleObject *)mo)->path = atom(path);
    image_load_consts(image, _load_const_, mo);
    IMAGE_LOAD_VARS(image, _load_var_, mo);
    IMAGE_LOAD_CONSTVARS(image, _load_var_, mo);
    IMAGE_LOAD_FUNCS(image, _load_func_, mo);
    IMAGE_LOAD_NFUNCS(image, _load_nfunc_, mo);
    IMAGE_LOAD_CLASSES(image, _load_class_, mo);
    IMAGE_LOAD_TRAITS(image, _load_trait_, mo);
    IMAGE_LOAD_ENUMS(image, _load_enum_, mo);
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
    Object *mo = module_from_file(path, name, NULL);
    if (mo == NULL) {
      warn("cannot find module '%s'", path);
      return NULL;
    } else {
      module_install(path, mo);
      return mo;
    }
  } else {
    ModuleObject *mo = (ModuleObject *)node->ob;
    if (!mo->ready) {
      debug("module '%s' is not ready", mo->name);
      char *name = strrchr(path, '/');
      name = name ? name + 1 : path;
      Object *ob = module_from_file(path, name, node->ob);
      if (ob != NULL) {
        mo->ready = 1;
        if (!compflag) {
          run_func(ob, "__init__", NULL);
        }
      }
    }
  }
  return OB_INCREF(node->ob);
}
