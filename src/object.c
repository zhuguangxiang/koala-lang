/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <inttypes.h>
#include "moduleobject.h"
#include "tupleobject.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "classobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "floatobject.h"
#include "atom.h"
#include "log.h"

int mnode_equal(void *e1, void *e2)
{
  struct mnode *n1 = e1;
  struct mnode *n2 = e2;
  return (n1 == n2) || !strcmp(n1->name, n2->name);
}

struct mnode *mnode_new(char *name, Object *ob)
{
  struct mnode *node = kmalloc(sizeof(*node));
  if (!node)
    panic("memory allocation failed.");
  node->name = name;
  hashmap_entry_init(node, strhash(name));
  node->obj = OB_INCREF(ob);
  return node;
}

void mnode_free(void *e, void *arg)
{
  struct mnode *node = e;
  OB_DECREF(node->obj);
  kfree(node);
}

static HashMap *get_mtbl(TypeObject *type)
{
  HashMap *mtbl = type->mtbl;
  if (mtbl == NULL) {
    mtbl = kmalloc(sizeof(*mtbl));
    if (!mtbl)
      panic("memory allocation failed.");
    hashmap_init(mtbl, mnode_equal);
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
  if (OB_TYPE(self) != OB_TYPE(other))
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

static Object *any_class(Object *ob, Object *args)
{
  if (ob == NULL) {
    error("object is NULL");
    return NULL;
  }

  if (args != NULL) {
    error("'__class__' has no arguments");
    return NULL;
  }

  return Class_New(ob);
}

TypeObject Any_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name   = "Any",
  .hash   = any_hash,
  .equal  = any_equal,
  .clazz  = any_class,
  .str    = any_str,
};

static int lro_find(Vector *vec, TypeObject *type)
{
  TypeObject *item;
  VECTOR_ITERATOR(iter, vec);
  iter_for_each(&iter, item) {
    if (item == type)
      return 1;
  }
  return 0;
}

static void lro_build_one(TypeObject *type, TypeObject *base)
{
  Vector *vec = &type->lro;

  TypeObject *item;
  VECTOR_ITERATOR(iter, &base->lro);
  iter_for_each(&iter, item) {
    if (!lro_find(vec, item)) {
      vector_push_back(vec, item);
    }
  }
  if (!lro_find(vec, base)) {
    vector_push_back(vec, base);
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
  iter_for_each(&iter, base) {
    lro_build_one(type, base);
  }

  /* add self */
  lro_build_one(type, type);

  return 0;
}

static void destroy_lro(TypeObject *type)
{
  vector_fini(&type->lro, NULL, NULL);
}

static void type_show(TypeObject *type)
{
  print("#\n");
  print("%s: (", type->name);
  VECTOR_REVERSE_ITERATOR(iter, &type->lro);
  int size = vector_size(&type->lro);
  TypeObject *item;
  iter_for_each(&iter, item) {
    if (--size <= 0)
      print("'%s'", item->name);
    else
      print("'%s', ", item->name);
  }
  print(")\n");

  size = hashmap_size(type->mtbl);
  if (size > 0) {
    print("(");
    HASHMAP_ITERATOR(mapiter, type->mtbl);
    struct mnode *node;
    iter_for_each(&mapiter, node) {
      if (--size <= 0)
        print("'%s'", node->name);
      else
        print("'%s', ", node->name);
    }
    print(")\n");
  }
}

int Type_Ready(TypeObject *type)
{
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

void Type_Fini(TypeObject *type)
{
  destroy_lro(type);

  HashMap *map = type->mtbl;
  if (map != NULL) {
    debug("fini type '%s'", type->name);
    hashmap_fini(map, mnode_free, NULL);
    debug("------------------------");
    kfree(map);
    type->mtbl = NULL;
  }
}

void Type_Add_Field(TypeObject *type, Object *ob)
{
  FieldObject *field = (FieldObject *)ob;
  field->owner = (Object *)type;
  field->offset = type->nrvars;
  ++type->nrvars;
  struct mnode *node = mnode_new(field->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  if (res != 0)
    panic("'%.64s' add '%.64s' failed.", type->name, field->name);
}

void Type_Add_FieldDef(TypeObject *type, FieldDef *f)
{
  TypeDesc *desc = string_to_desc(f->type);
  Object *field = Field_New(f->name, desc);
  TYPE_DECREF(desc);
  Field_SetFunc(field, f->set, f->get);
  Type_Add_Field(type, field);
  OB_DECREF(field);
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
  meth->owner = (Object *)type;
  struct mnode *node = mnode_new(meth->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  if (res != 0)
    panic("'%.64s' add '%.64s' failed.", type->name, meth->name);
}

void Type_Add_MethodDef(TypeObject *type, MethodDef *f)
{
  Object *meth = CMethod_New(f);
  Type_Add_Method(type, meth);
  OB_DECREF(meth);
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

  VECTOR_REVERSE_ITERATOR(iter, &type->lro);
  TypeObject *item;
  struct mnode *node;
  iter_for_each(&iter, item) {
    if (item->mtbl == NULL)
      continue;
    node = hashmap_get(item->mtbl, &key);
    if (node != NULL)
      return OB_INCREF(node->obj);
  }
  return NULL;
}

unsigned int Object_Hash(Object *ob)
{
  Object *res = Object_Call(ob, "__hash__", NULL);
  if (!res)
    panic("'__hash__' is not implemented");
  unsigned int hash = Integer_AsInt(res);
  OB_DECREF(res);
  return hash;
}

int Object_Equal(Object *ob1, Object *ob2)
{
  if (ob1 == ob2)
    return 1;

  TypeObject *type1 = OB_TYPE(ob1);
  TypeObject *type2 = OB_TYPE(ob2);
  if (type1 != type2)
    return 0;
  Object *ob = Object_Call(ob1, "__equal__", ob2);
  if (ob == NULL)
    return 0;
  return Bool_IsTrue(ob) ? 1 : 0;
}

Object *Object_Class(Object *ob)
{

}

Object *Object_String(Object *ob)
{

}

Object *Object_Lookup(Object *self, char *name)
{
  Object *res;
  if (Module_Check(self)) {
    res = Module_Lookup(self, name);
  } else {
    res = Type_Lookup(OB_TYPE(self), name);
  }
  return res;
}

Object *Object_GetMethod(Object *self, char *name)
{
  Object *ob = Object_Lookup(self, name);
  if (Method_Check(ob)) {
    return ob;
  } else {
    error("'%s' is not a Method", name);
    OB_DECREF(ob);
    return NULL;
  }
}

Object *Object_GetField(Object *self, char *name)
{
  Object *ob = Object_Lookup(self, name);
  if (Field_Check(ob)) {
    return ob;
  } else {
    error("'%s' is not a Field", name);
    OB_DECREF(ob);
    return NULL;
  }
}

Object *Object_Call(Object *self, char *name, Object *args)
{
  Object *ob = Object_Lookup(self, name);
  if (!ob)
    panic("object of '%.64s' has no '%.64s'", OB_TYPE_NAME(self), name);
  if (Type_Check(ob)) {
    ob = Object_Lookup(ob, "__call__");
    if (!ob)
      panic("object of '%.64s' has no '__call__'", OB_TYPE_NAME(self));
  }
  Object *res = Method_Call(ob, self, args);
  OB_DECREF(ob);
  return res;
}

Object *Object_GetValue(Object *self, char *name)
{
  Object *res = NULL;
  Object *ob = Object_Lookup(self, name);
  if (ob == NULL) {
    error("object of '%.64s' has no field '%.64s'", OB_TYPE_NAME(self), name);
    return NULL;
  }

  if (!Field_Check(ob)) {
    if (Method_Check(ob)) {
      /* if method has no any parameters, it can be accessed as field. */
      MethodObject *meth = (MethodObject *)ob;
      TypeDesc *desc = meth->desc;
      if (!desc->proto.args) {
        res = Method_Call(ob, self, NULL);
        OB_DECREF(ob);
        return res;
      }
    }
    OB_DECREF(ob);
    error("'%s' is not a field", name);
    return NULL;
  }
  res = Field_Get(ob, self);
  OB_DECREF(ob);
  return res;
}

int Object_SetValue(Object *self, char *name, Object *val)
{
  Object *ob = Object_Lookup(self, name);
  if (ob == NULL) {
    error("object of '%.64s' has no field '%.64s'", OB_TYPE_NAME(self), name);
    return -1;
  }

  if (!Field_Check(ob)) {
    error("'%s' is not a field", name);
    OB_DECREF(ob);
    return -1;
  }

  Field_Set(ob, self, val);
  OB_DECREF(ob);
  return 0;
}

Object *New_Const(ConstValue *val)
{
  Object *ob = NULL;
  switch (val->kind) {
  case BASE_INT:
    debug("const int value: %ld", val->ival);
    ob = Integer_New(val->ival);
    break;
  case BASE_STR:
    debug("const string value: %s", val->str);
    ob = String_New(val->str);
    break;
  case BASE_BOOL:
    debug("const bool value: %s", val->bval ? "true" : "false");
    ob = val->bval ? Bool_True() : Bool_False();
    break;
  case BASE_BYTE:
    debug("const byte value: %ld", val->ival);
    ob = Byte_New((int)val->ival);
    break;
  case BASE_FLOAT:
    debug("const string value: %lf", val->fval);
    ob = Float_New(val->fval);
    break;
  case BASE_CHAR:
    debug("const string value: %s", (char *)&val->cval);
    ob = Char_New(val->cval.val);
    break;
  default:
    panic("invalid branch %d", val->kind);
    break;
  }
  return ob;
}
