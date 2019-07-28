/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "stringobject.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "intobject.h"
#include "atom.h"
#include "log.h"

int mnode_compare(void *e1, void *e2)
{
  struct mnode *n1 = e1;
  struct mnode *n2 = e2;
  return (n1 == n2) || strcmp(n1->name, n2->name);
}

struct mnode *mnode_new(char *name, Object *ob)
{
  struct mnode *node = kmalloc(sizeof(*node));
  panic(!node, "memory allocation failed.");
  node->name = name;
  hashmap_entry_init(node, strhash(name));
  node->obj = ob;
  return node;
}

static struct hashmap *get_mtbl(TypeObject *type)
{
  struct hashmap *mtbl = type->mtbl;
  if (mtbl == NULL) {
    mtbl = kmalloc(sizeof(*mtbl));
    panic(!mtbl, "memory allocation failed.");
    hashmap_init(mtbl, mnode_compare);
    type->mtbl = mtbl;
  }
  return mtbl;
}

static int lro_find(struct vector *vec, TypeObject *type)
{
  TypeObject *item;
  VECTOR_ITERATOR(iter, vec);
  iter_for_each_as(&iter, TypeObject *, item) {
    if (item == type)
      return 1;
  }
  return 0;
}

static void lro_build_one(TypeObject *type, TypeObject *base)
{
  struct vector *vec = type->lro;
  if (vec == NULL) {
    vec = kmalloc(sizeof(*vec));
    vector_init(vec, sizeof(void *));
    type->lro = vec;
  }

  TypeObject *item;
  VECTOR_ITERATOR(iter, base->lro);
  iter_for_each_as(&iter, TypeObject *, item) {
    if (!lro_find(vec, item)) {
      vector_push_back(vec, &item);
    }
  }
  if (!lro_find(vec, base)) {
    vector_push_back(vec, &base);
  }
  type->offset += base->offset;
}

static void lro_debug(TypeObject *type)
{
  debug("--------%s:line-order--------", type->name);
  VECTOR_REVERSE_ITERATOR(iter, type->lro);
  TypeObject *item;
  iter_for_each_as(&iter, TypeObject *, item) {
    debug("%s", item->name);
  }
  debug("-------------------------------");
}

static int build_lro(TypeObject *type)
{
  /* add Any */
  lro_build_one(type, &Any_Type);

  /* add base classes */
  VECTOR_ITERATOR(iter, type->bases);
  TypeObject *base;
  iter_for_each_as(&iter, TypeObject *, base) {
    lro_build_one(type, base);
  }

  /* add self */
  lro_build_one(type, type);

  /* show lro */
  lro_debug(type);
  return 0;
}

int Type_Add_CFunc(TypeObject *type, char *name,
                   char *ptype, char *rtype, cfunc func)
{
  Object *method = CMethod_New(name, ptype, rtype, func);
  int res = Type_Add_Method(type, method);
  panic(res, "'%s' add '%s' failed.", type->name, name);
  return res;
}

int Type_Ready(TypeObject *type)
{
  int res;
  Object *ob;

  if (type->hashfunc && !type->cmpfunc) {
    error("__cmp__ must be implemented, when __hash__ is implemented of '%s'.",
          type->name);
    return -1;
  }

  if (type->hashfunc != NULL) {
    Type_Add_CFunc(type, "__hash__", NULL, "i", type->hashfunc);
  }

  if (type->cmpfunc != NULL) {
    Type_Add_CFunc(type, "__cmp__", "A", "i", type->cmpfunc);
  }

  if (type->classfunc != NULL) {
    Type_Add_CFunc(type, "__class__", NULL, "Llang.Class", type->classfunc);
  }

  if (type->strfunc != NULL) {
    Type_Add_CFunc(type, "__str__", NULL, "s", type->strfunc);
  }

  if (type->fields != NULL) {
    FieldDef *f = type->fields;
    while (f->name != NULL) {
      ob = Field_New(f->name, f->type, f->getfunc, f->setfunc);
      res = Type_Add_Field(type, ob);
      panic(res, "'%s' add '%s' failed.", type->name, f->name);
      ++f;
    }
  }

  if (type->methods != NULL) {
    MethodDef *m = type->methods;
    while (m->name != NULL) {
      Type_Add_CFunc(type, m->name, m->ptype, m->rtype, m->func);
      ++m;
    }
  }

  if (build_lro(type) < 0)
    return -1;

  return 0;
}

int Type_Add_Field(TypeObject *type, Object *ob)
{
  FieldObject *field = (FieldObject *)ob;
  struct mnode *node = mnode_new(field->name, ob);
  field->owner = type;
  return hashmap_add(get_mtbl(type), node);
}

int Type_Add_Method(TypeObject *type, Object *ob)
{
  MethodObject *method = (MethodObject *)ob;
  struct mnode *node = mnode_new(method->name, ob);
  method->owner = type;
  return hashmap_add(get_mtbl(type), node);
}

/* look in type->dict and its bases */
Object *Type_Get(TypeObject *self, char *name)
{
  if (!Type_Check(self)) {
    error("object of '%.64s' is not a type.", name);
    return NULL;
  }

  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));

  VECTOR_REVERSE_ITERATOR(iter, self->lro);
  TypeObject *item;
  iter_for_each_as(&iter, TypeObject *, item) {
    struct mnode *node = hashmap_get(item->mtbl, &key);
    if (node != NULL) {
      debug("found '%s' in '%s'", name, item->name);
      return node->obj;
    }
  }

  error("cannot find '%s' in '%s'.", name, self->name);
  return NULL;
}

static Object *_type_get_cb_(Object *ob, Object *args)
{
  return Type_Get((TypeObject *)ob, String_AsStr(args));
}

TypeObject Class_Type = {
  OBJECT_HEAD_INIT(&Class_Type)
  .name = "Class",
  .getfunc = _type_get_cb_,
};

static Object *_any_hash_cb_(Object *self, Object *args)
{
  unsigned int hash = memhash(&self, sizeof(void *));
  return Integer_New(hash);
}

static Object *_any_cmp_cb_(Object *self, Object *other)
{
  int res = Type_Equal(OB_TYPE(self), OB_TYPE(other));
  res = !res ? -1 : !(self == other);
  return Integer_New(res);
}

static Object *_any_str_cb_(Object *ob, Object *args)
{
  char buf[80];
  snprintf(buf, sizeof(buf)-1, "%.64s@%lx", OB_TYPE(ob)->name, (uintptr_t)ob);
  return String_New(buf);
}

static Object *_any_class_cb_(Object *ob, Object *args)
{
  return (Object *)OB_TYPE(ob);
}

TypeObject Any_Type = {
  OBJECT_HEAD_INIT(&Class_Type)
  .name = "Any",
  .hashfunc  = _any_hash_cb_,
  .cmpfunc   = _any_cmp_cb_,
  .strfunc   = _any_str_cb_,
  .classfunc = _any_class_cb_,
};

Object *Object_Get(Object *self, char *name)
{
  Object *member = Type_Get(OB_TYPE(self), name);
  if (!member) {
    error("cannot find '%.64s' in '%.64s' type.", name, OB_TYPE(self)->name);
    return NULL;
  }

  getfunc get = OB_TYPE(member)->getfunc;
  if (!get) {
    error("member '%.64s' is not getterable.", name);
    return NULL;
  }

  return get(member, self);
}

int Object_Set(Object *self, char *name, Object *val)
{
  Object *member = Type_Get(OB_TYPE(self), name);
  if (!member) {
    error("cannot find '%.64s' in '%.64s' type.", name, OB_TYPE(self)->name);
    return 0;
  }

  setfunc set = OB_TYPE(member)->setfunc;
  if (!set) {
    error("member '%.64s' is not setterable.", name);
    return 0;
  }

  return set(member, self, val);
}
