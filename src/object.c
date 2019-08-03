/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "tupleobject.h"
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

static Object *any_hash(Object *self, Object *args)
{
  unsigned int hash = memhash(&self, sizeof(void *));
  return Integer_New(hash);
}

static Object *any_equal(Object *self, Object *other)
{
  if (!Type_Equal(OB_TYPE(self), OB_TYPE(other)))
    return Bool_False();
  return (self == other) ? Bool_True() : Bool_False();
}

static Object *any_str(Object *ob, Object *args)
{
  char buf[80];
  snprintf(buf, sizeof(buf)-1, "%.64s@%lx",
           OB_TYPE_NAME(ob), (uintptr_t)ob);
  return String_New(buf);
}

static Object *any_fmt(Object *ob, Object *args)
{

}

TypeObject Any_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name   = "Any",
  .lookup = Object_Lookup,
  .hash   = any_hash,
  .equal  = any_equal,
  .clazz  = Object_Class,
  .str    = any_str,
  //.fmt    = any_fmt,
};

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

  return 0;
}

static void type_show(TypeObject *type)
{
  print("------------------------\n");
  print("%s:\n", type->name);
  VECTOR_REVERSE_ITERATOR(iter, type->lro);
  int size = vector_size(type->lro);
  TypeObject *item;
  print("LRO: (");
  iter_for_each_as(&iter, TypeObject *, item) {
    if (--size <= 0)
      print("'%s'", item->name);
    else
      print("'%s', ", item->name);
  }
  print(")\n");

  print("MBR: (");
  HASHMAP_ITERATOR(mapiter, type->mtbl);
  size = hashmap_size(type->mtbl);
  struct mnode *node;
  iter_for_each(&mapiter, node) {
    if (--size <= 0)
      print("'%s'", node->name);
    else
      print("'%s', ", node->name);
  }
  print(")\n");
}

int Type_Ready(TypeObject *type)
{
  int res;
  Object *ob;

  if (type->hash && !type->equal) {
    error("__equal__ must be implemented, "
          "when __hash__ is implemented of '%.64s'",
          type->name);
    return -1;
  }

  if (type->hash != NULL) {
    MethodDef meth = {"__hash__", NULL, "i", type->hash};
    Type_Add_MethodDef(type, &meth);
  }

  if (type->equal != NULL) {
    MethodDef meth = {"__equal__", "A", "i", type->equal};
    Type_Add_MethodDef(type, &meth);
  }

  if (type->clazz != NULL) {
    MethodDef meth = {"__class__", NULL, "Llang.Class;", type->clazz};
    Type_Add_MethodDef(type, &meth);
  }

  if (type->str != NULL) {
    MethodDef meth = {"__str__", NULL, "s", type->str};
    Type_Add_MethodDef(type, &meth);
  }

  if (type->methods != NULL) {
    Type_Add_MethodDefs(type, type->methods);
  }

  if (build_lro(type) < 0)
    return -1;

  type_show(type);

  return 0;
}

void Type_Add_Field(TypeObject *type, Object *ob)
{
  FieldObject *field = (FieldObject *)ob;
  field->owner = (Object *)OB_INCREF(type);
  struct mnode *node = mnode_new(field->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  panic(res, "'%.64s' add '%.64s' failed.", type->name, field->name);
}

void Type_Add_FieldDef(TypeObject *type, FieldDef *f)
{
  Object *field = Field_New(f);
  Type_Add_Field(type, field);
}

void Type_Add_FieldDefs(TypeObject *type, FieldDef *def)
{
  FieldDef *f = def;
  while (f->name) {
    Type_Add_FieldDef(type, f);
    ++f;
  }
}

void Type_Add_Method(TypeObject *type, Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  meth->owner = (Object *)OB_INCREF(type);
  struct mnode *node = mnode_new(meth->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  panic(res, "'%.64s' add '%.64s' failed.", type->name, meth->name);
}

void Type_Add_MethodDef(TypeObject *type, MethodDef *f)
{
  Object *meth = CMethod_New(f);
  Type_Add_Method(type, meth);
}

void Type_Add_MethodDefs(TypeObject *type, MethodDef *def)
{
  MethodDef *f = def;
  while (f->name) {
    Type_Add_MethodDef(type, f);
    ++f;
  }
}

TypeObject Type_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name = "Type",
};

/* look in type->mtbl and its bases */
Object *Type_Lookup(TypeObject *type, char *name)
{
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));

  VECTOR_REVERSE_ITERATOR(iter, type->lro);
  TypeObject *item;
  struct mnode *node;
  iter_for_each_as(&iter, TypeObject *, item) {
    if (item->mtbl == NULL)
      continue;
    node = hashmap_get(item->mtbl, &key);
    if (node != NULL)
      return node->obj;
  }
  return NULL;
}

Object *Object_Lookup(Object *self, char *name)
{
  TypeObject *type = OB_TYPE(self);
  return Type_Lookup(type, name);
}

Object *Object_Class(Object *ob, Object *args)
{
  if (ob == NULL) {
    error("object is NULL");
    return NULL;
  }

  if (args != NULL) {
    error("__class__ has no arguments");
    return NULL;
  }

  return Class_New(ob);
}

int Object_Hash(Object *ob, unsigned int *hash)
{
  Object *res = Object_Call(ob, "__hash__", NULL);
  if (res != NULL) {
    *hash = Integer_AsInt(res);
    OB_DECREF(res);
    return 0;
  }
  return -1;
}

int Object_Equal(Object *ob1, Object *ob2)
{
  if (ob1 == ob2)
    return 1;

  TypeObject *type1 = OB_TYPE(ob1);
  TypeObject *type2 = OB_TYPE(ob2);
  if (!Type_Equal(type1, type2))
    return 0;
  Object *ob = Object_Call(ob1, "__equal__", ob2);
  if (ob == NULL)
    return 0;
  return Bool_IsTrue(ob) ? 1 : 0;
}

Object *Object_Call(Object *self, char *name, Object *args)
{
  TypeObject *type = OB_TYPE(self);
  Object *ob = type->lookup(self, name);
  panic(!ob, "Object of '%.64s' has not '%.64s' method",
        type->name, name);
  return Method_Call(ob, self, args);
}

Object *Object_Get(Object *self, char *name)
{
  Object *ob = OB_TYPE(self)->lookup(self, name);
  if (ob == NULL) {
    error("object of '%.64s' has not field '%.64s'",
          OB_TYPE(self)->name, name);
    return NULL;
  }

  if (!Field_Check(ob)) {
    if (Method_Check(ob)) {
      /* if method has no any parameters, it can be accessed as field. */
      MethodObject *meth = (MethodObject *)ob;
      ProtoDesc *proto = (ProtoDesc *)meth->desc;
      if (!proto->args)
        return Method_Call(ob, self, NULL);
    }

    error("'%s' is not a field", name);
    return NULL;
  }

  FieldObject *field = (FieldObject *)ob;
  return field->get(ob, self);
}

int Object_Set(Object *self, char *name, Object *val)
{
  Object *ob = OB_TYPE(self)->lookup(self, name);
  if (ob == NULL) {
    error("object of '%.64s' has not field '%.64s'",
          OB_TYPE(self)->name, name);
    return -1;
  }

  if (!Field_Check(ob)) {
    error("'%s' is not a field", name);
    return -1;
  }

  FieldObject *field = (FieldObject *)ob;
  Object *tuple = Tuple_Pack(2, self, val);
  field->set(ob, tuple);
  OB_DECREF(tuple);
  return 0;
}
