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
    hashmap_init(mtbl, mnode_equal);
    type->mtbl = mtbl;
  }
  return mtbl;
}

static Object *any_hash(Object *self, Object *args)
{
  unsigned int hash = memhash(&self, sizeof(void *));
  return integer_new(hash);
}

static Object *any_cmp(Object *self, Object *other)
{
  if (OB_TYPE(self) != OB_TYPE(other))
    return Bool_False();
  return (self == other) ? Bool_True() : Bool_False();
}

static Object *any_str(Object *ob, Object *args)
{
  char buf[64];
  snprintf(buf, sizeof(buf) - 1, "%.32s@%lx",
           OB_TYPE_NAME(ob), (uintptr_t)ob);
  return string_new(buf);
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

TypeObject any_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Any",
  .hash  = any_hash,
  .cmp   = any_cmp,
  .clazz = any_class,
  .str   = any_str,
};

void init_any_type(void)
{
  any_type.desc = desc_from_any;
  if (type_ready(&any_type) < 0)
    panic("Cannot initalize 'Any' type.");
}

static int lro_find(Vector *vec, TypeObject *type)
{
  TypeObject *item;
  vector_for_each(item, vec) {
    if (item == type)
      return 1;
  }
  return 0;
}

static void lro_build_one(TypeObject *type, TypeObject *base)
{
  Vector *vec = &type->lro;

  TypeObject *item;
  vector_for_each(item, &base->lro) {
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
  lro_build_one(type, &any_type);

  /* add base classes */
  TypeObject *base;
  vector_for_each(base, type->bases) {
    lro_build_one(type, base);
  }

  /* add self */
  lro_build_one(type, type);

  return 0;
}

static void destroy_lro(TypeObject *type)
{
  vector_fini(&type->lro);
}

static void type_show(TypeObject *type)
{
  print("#\n");
  print("%s: (", type->name);
  int size = vector_size(&type->lro);
  TypeObject *item;
  vector_for_each_reverse(item, &type->lro) {
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

static void Type_Add_Numbers(TypeObject *type, NumberMethods *meths)
{
  MethodDef def;
  if (meths->add) {
    def.name = "__add__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->add;
    type_add_methoddef(type, &def);
  }
  if (meths->sub) {
    def.name = "__sub__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->sub;
    type_add_methoddef(type, &def);
  }
  if (meths->mul) {
    def.name = "__mul__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->mul;
    type_add_methoddef(type, &def);
  }
  if (meths->div) {
    def.name = "__div__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->div;
    type_add_methoddef(type, &def);
  }
  if (meths->mod) {
    def.name = "__mod__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->mod;
    type_add_methoddef(type, &def);
  }
  if (meths->pow) {
    def.name = "__pow__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->pow;
    type_add_methoddef(type, &def);
  }
  if (meths->neg) {
    def.name = "__neg__";
    def.ptype = NULL;
    def.rtype = "A";
    def.func = meths->neg;
    type_add_methoddef(type, &def);
  }
  if (meths->gt) {
    def.name = "__gt__";
    def.ptype = "A";
    def.rtype = "z";
    def.func = meths->gt;
    type_add_methoddef(type, &def);
  }
  if (meths->ge) {
    def.name = "__ge__";
    def.ptype = "A";
    def.rtype = "z";
    def.func = meths->ge;
    type_add_methoddef(type, &def);
  }
  if (meths->lt) {
    def.name = "__lt__";
    def.ptype = "A";
    def.rtype = "z";
    def.func = meths->lt;
    type_add_methoddef(type, &def);
  }
  if (meths->le) {
    def.name = "__le__";
    def.ptype = "A";
    def.rtype = "z";
    def.func = meths->le;
    type_add_methoddef(type, &def);
  }
  if (meths->eq) {
    def.name = "__eq__";
    def.ptype = "A";
    def.rtype = "z";
    def.func = meths->eq;
    type_add_methoddef(type, &def);
  }
  if (meths->neq) {
    def.name = "__neq__";
    def.ptype = "A";
    def.rtype = "z";
    def.func = meths->neq;
    type_add_methoddef(type, &def);
  }
  if (meths->and) {
    def.name = "__and__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->and;
    type_add_methoddef(type, &def);
  }
  if (meths->or) {
    def.name = "__or__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->or;
    type_add_methoddef(type, &def);
  }
  if (meths->xor) {
    def.name = "__xor__";
    def.ptype = "A";
    def.rtype = "A";
    def.func = meths->xor;
    type_add_methoddef(type, &def);
  }
  if (meths->not) {
    def.name = "__not__";
    def.ptype = NULL;
    def.rtype = "A";
    def.func = meths->not;
    type_add_methoddef(type, &def);
  }
}

static void Type_Add_Mapping(TypeObject *type, MappingMethods *meths)
{

}

int type_ready(TypeObject *type)
{
  if (type->hash && !type->cmp) {
    error("__cmp__ must be implemented, "
          "when __hash__ is implemented of '%s'",
          type->name);
    return -1;
  }

  if (type->hash != NULL) {
    MethodDef meth = {"__hash__", NULL, "i", type->hash};
    type_add_methoddef(type, &meth);
  }

  if (type->cmp != NULL) {
    MethodDef meth = {"__cmp__", "A", "i", type->cmp};
    type_add_methoddef(type, &meth);
  }

  if (type->clazz != NULL) {
    MethodDef meth = {"__class__", NULL, "Llang.Class;", type->clazz};
    type_add_methoddef(type, &meth);
  }

  if (type->str != NULL) {
    MethodDef meth = {"__str__", NULL, "s", type->str};
    type_add_methoddef(type, &meth);
  }

  if (type->methods != NULL) {
    Type_Add_MethodDefs(type, type->methods);
  }

  if (build_lro(type) < 0)
    return -1;

  type_show(type);

  return 0;
}

void type_fini(TypeObject *type)
{
  destroy_lro(type);
  TYPE_DECREF(type->desc);

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
  expect(res == 0);
}

void Type_Add_FieldDef(TypeObject *type, FieldDef *f)
{
  TypeDesc *desc = str_to_desc(f->type);
  Object *field = field_new(f->name, desc);
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
  expect(res == 0);
}

static int get_para_index(Vector *vec, char *name)
{
  TypeDesc *def;
  vector_for_each(def, vec) {
    if (!strcmp(def->paradef.name, name))
      return idx;
  }
  return -1;
}

static void update_pararef(TypeDesc *para, TypeDesc *proto)
{
  int index;
  TypeDesc *rtype = proto->proto.ret;
  if (rtype != NULL) {
    if (rtype->kind == TYPE_PARAREF) {
      index = get_para_index(para->paras, rtype->pararef.name);
      expect(index >= 0);
      rtype->pararef.index = index;
    } else if (rtype->kind == TYPE_KLASS) {
      TypeDesc *item;
      vector_for_each(item, rtype->types) {
        if (item->kind == TYPE_PARAREF) {
          index = get_para_index(para->paras, item->pararef.name);
          expect(index >= 0);
          item->pararef.index = index;
        } else {
          expect(item->kind != TYPE_PARADEF);
        }
      }
    }
  }

  TypeDesc *ptype;
  vector_for_each(ptype, proto->proto.args) {
    if (ptype->kind == TYPE_PARAREF) {
      index = get_para_index(para->paras, ptype->pararef.name);
      expect(index >= 0);
      ptype->pararef.index = index;
    }
  }
}

void type_add_methoddef(TypeObject *type, MethodDef *f)
{
  Object *meth = CMethod_New(f);
  update_pararef(type->desc, ((MethodObject *)meth)->desc);
  Type_Add_Method(type, meth);
  OB_DECREF(meth);
}

void Type_Add_MethodDefs(TypeObject *type, MethodDef *def)
{
  MethodDef *f = def;
  while (f->name) {
    type_add_methoddef(type, f);
    ++f;
  }
}

TypeObject type_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Type",
};

/* look in type->mtbl and its bases */
Object *Type_Lookup(TypeObject *type, char *name)
{
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));

  TypeObject *item;
  struct mnode *node;
  vector_for_each_reverse(item, &type->lro) {
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
  expect(res != NULL);
  unsigned int hash = integer_asint(res);
  OB_DECREF(res);
  return hash;
}

int Object_Cmp(Object *ob1, Object *ob2)
{
  if (ob1 == ob2)
    return 1;

  TypeObject *type1 = OB_TYPE(ob1);
  TypeObject *type2 = OB_TYPE(ob2);
  if (type1 != type2)
    return 0;
  Object *ob = Object_Call(ob1, "__cmp__", ob2);
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
  if (method_check(ob)) {
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
  if (field_check(ob)) {
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
  expect(ob != NULL);
  if (Type_Check(ob)) {
    ob = Object_Lookup(ob, "__call__");
    expect(ob != NULL);
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
    error("object of '%s' has no field '%s'", OB_TYPE_NAME(self), name);
    return NULL;
  }

  if (!field_check(ob)) {
    if (method_check(ob)) {
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
    error("'%s' is not getable", name);
    return NULL;
  }
  res = field_get(ob, self);
  OB_DECREF(ob);
  return res;
}

int Object_SetValue(Object *self, char *name, Object *val)
{
  Object *ob = Object_Lookup(self, name);
  if (ob == NULL) {
    error("object of '%s' has no field '%s'", OB_TYPE_NAME(self), name);
    return -1;
  }

  if (!field_check(ob)) {
    error("'%s' is not setable", name);
    OB_DECREF(ob);
    return -1;
  }

  int res = field_set(ob, self, val);
  OB_DECREF(ob);
  return res;
}

Object *new_literal(Literal *val)
{
  Object *ob = NULL;
  switch (val->kind) {
  case BASE_INT:
    debug("literal int value: %ld", val->ival);
    ob = integer_new(val->ival);
    break;
  case BASE_STR:
    debug("literal string value: %s", val->str);
    ob = string_new(val->str);
    break;
  case BASE_BOOL:
    debug("literal bool value: %s", val->bval ? "true" : "false");
    ob = val->bval ? Bool_True() : Bool_False();
    break;
  case BASE_BYTE:
    debug("literal byte value: %ld", val->ival);
    ob = Byte_New((int)val->ival);
    break;
  case BASE_FLOAT:
    debug("literal string value: %lf", val->fval);
    ob = Float_New(val->fval);
    break;
  case BASE_CHAR:
    debug("literal string value: %s", (char *)&val->cval);
    ob = Char_New(val->cval.val);
    break;
  default:
    panic("invalid literal %d", val->kind);
    break;
  }
  return ob;
}

static void descob_free(Object *ob)
{
  if (!descob_check(ob)) {
    error("object of '%s' is not a TypeDesc", OB_TYPE_NAME(ob));
    return;
  }
  DescObject *descob = (DescObject *)ob;
  TYPE_DECREF(descob->desc);
  kfree(ob);
}

TypeObject descob_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "TypeDesc",
  .free = descob_free,
};

Object *new_descob(TypeDesc *desc)
{
  DescObject *descob = kmalloc(sizeof(DescObject));
  init_object_head(descob, &descob_type);
  descob->desc = TYPE_INCREF(desc);
  return (Object *)descob;
}

void init_descob_type(void)
{
  descob_type.desc = desc_from_desc;
  if (type_ready(&descob_type) < 0)
    panic("Cannot initalize 'TypeDesc' type.");
}
