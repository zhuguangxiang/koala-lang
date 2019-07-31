/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "fieldobject.h"
#include "methodobject.h"
#include "classobject.h"
#include "stringobject.h"
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

int Type_Ready(TypeObject *type)
{
  int res;
  Object *ob;

  if (type->hashfunc && !type->equalfunc) {
    error("__equal__ must be implemented, when __hash__ is implemented of '%s'.",
          type->name);
    return -1;
  }

  if (type->hashfunc != NULL) {
    MethodDef meth = {"__hash__", NULL, "i", type->hashfunc};
    Type_Add_CFunc(type, &meth);
  }

  if (type->equalfunc != NULL) {
    MethodDef meth = {"__equal__", "A", "i", type->equalfunc};
    Type_Add_CFunc(type, &meth);
  }

  if (type->classfunc != NULL) {
    MethodDef meth = {"__class__", NULL, "Llang.Class", type->classfunc};
    Type_Add_CFunc(type, &meth);
  }

  if (type->strfunc != NULL) {
    MethodDef meth = {"__str__", NULL, "s", type->strfunc};
    Type_Add_CFunc(type, &meth);
  }

  if (type->fields != NULL) {
    FieldDef *f = type->fields;
    Object *ob;
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
      debug("add method '%s' in '%s'", m->name, type->name);
      Type_Add_CFunc(type, m);
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
  field->owner = (Object *)type;
  return hashmap_add(get_mtbl(type), node);
}

int Type_Add_Method(TypeObject *type, Object *ob)
{
  MethodObject *method = (MethodObject *)ob;
  struct mnode *node = mnode_new(method->name, ob);
  method->owner = (Object *)type;
  return hashmap_add(get_mtbl(type), node);
}

int Type_Add_CFunc(TypeObject *type, MethodDef *f)
{
  Object *method = CMethod_New(f->name, f->ptype, f->rtype, f->func);
  int res = Type_Add_Method(type, method);
  panic(res, "'%s' add '%s' failed.", type->name, f->name);
  return res;
}

TypeObject Type_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Type",
};

static Object *_any_hash_(Object *self, Object *args)
{
  unsigned int hash = memhash(&self, sizeof(void *));
  return Integer_New(hash);
}

static Object *_any_equal_(Object *self, Object *other)
{
  int res = Type_Equal(OB_TYPE(self), OB_TYPE(other));
  res = !res ? -1 : !(self == other);
  return Integer_New(res);
}

static Object *_any_str_(Object *ob, Object *args)
{
  char buf[80];
  snprintf(buf, sizeof(buf)-1, "%.64s@%lx", OB_TYPE(ob)->name, (uintptr_t)ob);
  return String_New(buf);
}

TypeObject Any_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Any",
  .getfunc   = _object_member_,
  .hashfunc  = _any_hash_,
  .equalfunc = _any_equal_,
  .classfunc = _object_class_,
  .strfunc   = _any_str_,
};

/* look in type->dict and its bases */
Object *_object_member_(Object *self, char *name)
{
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));

  TypeObject *type = OB_TYPE(self);
  VECTOR_REVERSE_ITERATOR(iter, type->lro);
  TypeObject *item;
  iter_for_each_as(&iter, TypeObject *, item) {
    struct mnode *node = hashmap_get(item->mtbl, &key);
    if (node != NULL) {
      debug("find '%s' in '%s'.", name, item->name);
      return node->obj;
    }
  }

  error("cannot find '%s' in '%s'.", name, type->name);
  return NULL;
}

Object *_object_class_(Object *ob, Object *args)
{
  if (!Field_Check(ob)) {
    error("object of '%.64s' is not a field.", OB_TYPE(ob)->name);
    return NULL;
  }

  ClassObject *cls = (ClassObject *)Class_New(args);
  cls->getfunc = _object_member_;
  return (Object *)cls;
}

Object *Object_CallMethod(Object *self, char *name, Object *args)
{
  TypeObject *type = OB_TYPE(self);
  Object *ob = type->getfunc(self, name);
  panic(!ob, "Object of '%s' has not 'get' method.", type->name);
  panic(!Method_Check(ob), "'%s' is not a method.", name);
  MethodObject *meth = (MethodObject *)ob;
  if (meth->cfunc) {
    cfunc fn = (cfunc)meth->ptr;
    return fn(self, args);
  } else {
    panic(1, "not implemented yet.");
    return NULL;
  }
}

Object *Object_GetValue(Object *self, char *name)
{
  Object *ob = OB_TYPE(self)->getfunc(self, name);
  if (!Field_Check(ob)) {
    error("object of '%s' has not field '%s'.", OB_TYPE(self)->name, name);
    return NULL;
  }

  return Object_CallMethod(ob, "get", self);
}
