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
  debug("[Freed] '%s'", node->name);
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

static Object *any_equal(Object *self, Object *other)
{
  if (OB_TYPE(self) != OB_TYPE(other))
    return bool_false();
  return (self == other) ? bool_true() : bool_false();
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
  return NULL;
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
  .equal = any_equal,
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
}

static int build_lro(TypeObject *type)
{
  /* add Any */
  lro_build_one(type, &any_type);

  /* add base classes */
  TypeObject *base;
  vector_for_each(base, &type->bases) {
    lro_build_one(type, base);
    type->offset += base->nrvars;
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
    size--;
    if (item == type)
      continue;
    if (size <= 0)
      print("'%s'", item->name);
    else
      print("'%s', ", item->name);
  }
  print(")\n");

  size = vector_size(type->tps);
  if (size > 0) {
    print("<");
    TypePara *tp;
    vector_for_each(tp, type->tps) {
      size--;
      if (size <= 0)
        print("%s", tp->name);
      else
        print("%s, ", tp->name);
    }
    print(">\n");
  }

  size = hashmap_size(type->mtbl);
  if (size > 0) {
    print("(");
    HASHMAP_ITERATOR(mapiter, type->mtbl);
    struct mnode *node;
    iter_for_each(&mapiter, node) {
      size--;
      if (size <= 0)
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
  if (type->hash && !type->equal) {
    error("__equal__ must be implemented, "
          "when __hash__ is implemented of '%s'",
          type->name);
    return -1;
  }

  if (type->hash != NULL) {
    MethodDef meth = {"__hash__", NULL, "i", type->hash};
    type_add_methoddef(type, &meth);
  }

  if (type->equal != NULL) {
    MethodDef meth = {"__equal__", "A", "i", type->equal};
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
    type_add_methoddefs(type, type->methods);
  }

  if (build_lro(type) < 0)
    return -1;

  type_show(type);

  return 0;
}

static Object *object_alloc(TypeObject *type)
{
  int isize = type->offset + type->nrvars;
  int msize = sizeof(HeapObject) + sizeof(Object *) * isize;
  HeapObject *ob = gcnew(msize);
  init_object_head(ob, type);
  ob->size = isize;
  return (Object *)ob;
}

static void object_clean(Object *self)
{
  debug("clean object '%s'", OB_TYPE_NAME(self));
  HeapObject *ob = (HeapObject *)self;
  Object *item;
  for (int i = 0; i < ob->size; i++) {
    item = ob->items[i];
    OB_DECREF(item);
  }
}

static void object_free(Object *self)
{
  object_clean(self);
  gcfree(self);
}

static void object_mark(Object *self)
{
  HeapObject *ob = (HeapObject *)self;
  Object *item;
  ob_markfunc mark;
  for (int i = 0; i < ob->size; i++) {
    item = ob->items[i];
    mark = OB_TYPE(item)->mark;
    if (mark != NULL)
      mark(item);
  }
}

TypeObject *type_new(char *path, char *name, int flags)
{
  TypeObject *tp = gcnew(sizeof(TypeObject));
  init_object_head(tp, &type_type);
  tp->name  = atom(name);
  tp->desc  = desc_from_klass(path, name);
  tp->flags = flags | TPFLAGS_GC;
  tp->mark  = object_mark,
  tp->clean = object_clean,
  tp->alloc = object_alloc;
  tp->free  = object_free;
  return tp;
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

  OB_DECREF(type->consts);
  vector_fini(&type->bases);
}

void type_add_field(TypeObject *type, Object *ob)
{
  FieldObject *field = (FieldObject *)ob;
  field->owner = (Object *)type;
  field->offset = type->nrvars++;
  struct mnode *node = mnode_new(field->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  expect(res == 0);
}

void type_add_field_default(TypeObject *type, char *name, TypeDesc *desc)
{
  Object *field = field_new(name, desc);
  Field_SetFunc(field, field_default_setter, field_default_getter);
  type_add_field(type, field);
  OB_DECREF(field);
}

void type_add_fielddef(TypeObject *type, FieldDef *f)
{
  TypeDesc *desc = str_to_desc(f->type);
  Object *field = field_new(f->name, desc);
  TYPE_DECREF(desc);
  Field_SetFunc(field, f->set, f->get);
  type_add_field(type, field);
  OB_DECREF(field);
}

void type_add_fielddefs(TypeObject *type, FieldDef *def)
{
  FieldDef *f = def;
  while (f->name) {
    type_add_fielddef(type, f);
    ++f;
  }
}

void type_add_method(TypeObject *type, Object *ob)
{
  MethodObject *meth = (MethodObject *)ob;
  meth->owner = (Object *)type;
  struct mnode *node = mnode_new(meth->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  expect(res == 0);
}

static int get_para_index(Vector *vec, char *name)
{
  TypePara *tp;
  vector_for_each(tp, vec) {
    if (!strcmp(tp->name, name))
      return idx;
  }
  return -1;
}

static void update_pararef(TypeObject *type, TypeDesc *desc)
{
  if (desc == NULL)
    return;

  if (desc->kind == TYPE_PARAREF) {
    int index = get_para_index(type->tps, desc->pararef.name);
    expect(index >= 0);
    desc->pararef.index = index;
    debug("%s: update_pararef: %s, %d", type->name, desc->pararef.name, index);
  } else if (desc->kind == TYPE_KLASS) {
    TypeDesc *item;
    vector_for_each(item, desc->klass.typeargs) {
      update_pararef(type, item);
    }
  } else {
    // not pararef, skip it.
  }
}

static void update_proto_pararef(TypeObject *type, TypeDesc *proto)
{
  update_pararef(type, proto->proto.ret);
  TypeDesc *desc;
  vector_for_each(desc, proto->proto.args) {
    update_pararef(type, desc);
  }
}

void type_add_methoddef(TypeObject *type, MethodDef *f)
{
  Object *meth = cmethod_new(f);
  debug("try to update func '%s' type parameters", f->name);
  update_proto_pararef(type, ((MethodObject *)meth)->desc);
  type_add_method(type, meth);
  OB_DECREF(meth);
}

void type_add_methoddefs(TypeObject *type, MethodDef *def)
{
  MethodDef *f = def;
  while (f->name) {
    type_add_methoddef(type, f);
    ++f;
  }
}

void type_add_ifunc(TypeObject *type, Object *ob)
{
  ProtoObject *proto = (ProtoObject *)ob;
  proto->owner = (Object *)type;
  struct mnode *node = mnode_new(proto->name, ob);
  int res = hashmap_add(get_mtbl(type), node);
  expect(res == 0);
}

static void type_clean(Object *ob)
{
  if (!type_check(ob)) {
    error("object of '%.64s' is not a Type", OB_TYPE_NAME(ob));
    return;
  }

  TypeObject *tp = (TypeObject *)ob;
  debug("clean type '%s'", tp->name);
  type_fini(tp);
}

static void type_free(Object *ob)
{
  TypeObject *tp = (TypeObject *)ob;
  type_clean(ob);
  debug("free type '%s'", tp->name);
  gcfree(ob);
}

static Object *type_str(Object *ob, Object *arg)
{
  TypeObject *tp = (TypeObject *)ob;
  return string_new(tp->name);
}

TypeObject type_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name  = "Type",
  .clean = type_clean,
  .free  = type_free,
  .str   = type_str,
};

TypeObject *type_parent(TypeObject *tp, TypeObject *base)
{
  TypeObject *item;
  vector_for_each_reverse(item, &tp->lro) {
    if (item == base) {
      expect(idx > 0);
      return vector_get(&tp->lro, idx - 1);
    }
  }
  return NULL;
}

/* look up type->mtbl only */
Object *type_lookup(TypeObject *type, TypeObject *start, char *name)
{
  struct mnode key = {.name = name};
  hashmap_entry_init(&key, strhash(name));

  int index = vector_size(&type->lro) - 1;
  debug("type '%s' search range is 0 ... %d", type->name, index);

  if (start != NULL) {
    TypeObject *item;
    vector_for_each_reverse(item, &type->lro) {
      if (item == start) {
        index = idx;
        break;
      }
    }
  }

  debug("type '%s' search start at %d", type->name, index);
  TypeObject *search;
  struct mnode *node;
  while (index >= 0) {
    search = vector_get(&type->lro, index);
    node = hashmap_get(search->mtbl, &key);
    if (node != NULL) return OB_INCREF(node->obj);
    --index;
  }

  warn("type '%s' search '%s' failed", type->name, name);
  return NULL;
}

unsigned int object_hash(Object *ob)
{
  Object *res = object_call(ob, "__hash__", NULL);
  expect(res != NULL);
  unsigned int hash = integer_asint(res);
  OB_DECREF(res);
  return hash;
}

int object_equal(Object *ob1, Object *ob2)
{
  if (ob1 == ob2)
    return 1;

  TypeObject *type1 = OB_TYPE(ob1);
  TypeObject *type2 = OB_TYPE(ob2);
  if (type1 != type2)
    return 0;
  Object *ob = object_call(ob1, "__equal__", ob2);
  if (ob == NULL)
    return 0;
  return bool_istrue(ob) ? 1 : 0;
}

Object *Object_Class(Object *ob)
{
  return NULL;
}

Object *Object_String(Object *ob)
{
  return NULL;
}

Object *object_lookup(Object *self, char *name, TypeObject *type)
{
  if (self == NULL) {
    return NULL;
  }

  Object *res;
  if (module_check(self)) {
    res = module_lookup(self, name);
  } else {
    res = type_lookup(OB_TYPE(self), type, name);
  }
  return res;
}

Object *object_getmethod(Object *self, char *name)
{
  Object *ob = object_lookup(self, name, NULL);
  if (method_check(ob)) {
    return ob;
  } else {
    error("'%s' is not a Method", name);
    OB_DECREF(ob);
    return NULL;
  }
}

Object *object_getfield(Object *self, char *name)
{
  Object *ob = object_lookup(self, name, NULL);
  if (field_check(ob)) {
    return ob;
  } else {
    error("'%s' is not a Field", name);
    OB_DECREF(ob);
    return NULL;
  }
}

Object *object_super_call(Object *self, char *name, Object *args,
                          TypeObject *type)
{
  Object *ob = object_lookup(self, name, type);
  expect(ob != NULL);
  Object *res = method_call(ob, self, args);
  OB_DECREF(ob);
  return res;
}

Object *object_call(Object *self, char *name, Object *args)
{
  if (self == NULL) {
    error("[Exception] null pointer");
    return NULL;
  }
  Object *ob = object_lookup(self, name, NULL);
  expect(ob != NULL);
  Object *res = method_call(ob, self, args);
  OB_DECREF(ob);
  return res;
}

Object *object_getvalue(Object *self, char *name, TypeObject *parent)
{
  Object *ob;
  if (type_check(self)) {
    // ENUM?
    TypeObject *type = (TypeObject *)self;
    ob = type_lookup(type, NULL, name);
    if (ob == NULL) {
      error("type of '%s' has no mbr '%s'", type->name, name);
      return NULL;
    } else {
      // no OB_INCREF
      return ob;
    }
  } else {
    ob = object_lookup(self, name, parent);
    if (ob == NULL) {
      error("object of '%s' has no field '%s'", OB_TYPE_NAME(self), name);
      return NULL;
    }
  }

  if (type_check(ob)) {
    TypeObject *type = (TypeObject *)ob;
    if (!type_isenum(ob)) {
      error("type of '%s' is not enum", type->name);
      return NULL;
    }
    // no OB_INCREF
    return ob;
  }

  Object *res = NULL;
  if (!field_check(ob)) {
    if (method_check(ob)) {
      /* if method has no any parameters, it can be accessed as field. */
      MethodObject *meth = (MethodObject *)ob;
      TypeDesc *desc = meth->desc;
      if (!desc->proto.args) {
        res = method_call(ob, self, NULL);
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

int object_setvalue(Object *self, char *name, Object *val, TypeObject *parent)
{
  Object *ob = object_lookup(self, name, parent);
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
    debug("literal int value: %"PRId64, val->ival);
    ob = integer_new(val->ival);
    break;
  case BASE_STR:
    debug("literal string value: %s", val->str);
    ob = string_new(val->str);
    break;
  case BASE_BOOL:
    debug("literal bool value: %s", val->bval ? "true" : "false");
    ob = val->bval ? bool_true() : bool_false();
    break;
  case BASE_BYTE:
    debug("literal byte value: %"PRId64, val->ival);
    ob = byte_new((int)val->ival);
    break;
  case BASE_FLOAT:
    debug("literal string value: %lf", val->fval);
    ob = Float_New(val->fval);
    break;
  case BASE_CHAR:
    debug("literal string value: %s", (char *)&val->cval);
    ob = char_new(val->cval.val);
    break;
  default:
    panic("invalid literal %d", val->kind);
    break;
  }
  return ob;
}

static void descob_clean(Object *ob)
{
  if (!descob_check(ob)) {
    error("object of '%s' is not a TypeDesc", OB_TYPE_NAME(ob));
    return;
  }
  DescObject *descob = (DescObject *)ob;
  TYPE_DECREF(descob->desc);
}

static void descob_free(Object *ob)
{
  descob_clean(ob);
  gcfree(ob);
}

TypeObject descob_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "TypeDesc",
  .clean = descob_clean,
  .free = descob_free,
};

Object *new_descob(TypeDesc *desc)
{
  DescObject *descob = gcnew(sizeof(DescObject));
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
