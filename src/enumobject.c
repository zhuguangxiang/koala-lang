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

#include "enumobject.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "hashmap.h"
#include "atom.h"
#include "log.h"

static void label_free(Object *ob)
{
  expect(label_check(ob));

  LabelObject *label = (LabelObject *)ob;
  debug("free label '%s'", label->name);

  free_descs(label->types);
  kfree(ob);
}

TypeObject label_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name = "Label",
  .free = label_free,
};

void init_label_type(void)
{
  label_type.desc = desc_from_klass("lang", "Label");
  if (type_ready(&label_type) < 0)
    panic("Cannot initalize 'Label' type.");
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

void type_add_label(TypeObject *type, char *name, Vector *_types)
{
  expect(type_isenum(type));
  debug("enum '%s' add label '%s'", type->name, name);

  LabelObject *label = kmalloc(sizeof(LabelObject));
  init_object_head(label, &label_type);
  label->name = atom(name);
  Vector *types = NULL;
  if (_types != NULL) {
    types = vector_new();
    TypeDesc *item;
    vector_for_each(item, _types) {
      vector_push_back(types, TYPE_INCREF(item));
    }
  }
  label->types = types;

  struct mnode *node = mnode_new(label->name, (Object *)label);
  int res = hashmap_add(get_mtbl(type), node);
  expect(res == 0);
  OB_DECREF(label);
}

static void enum_free(Object *ob)
{
  expect(type_isenum(OB_TYPE(ob)));

  EnumObject *eob = (EnumObject *)ob;
  debug("free enum object '%s' of '%s'", eob->name, OB_TYPE_NAME(ob));

  OB_DECREF(eob->values);
  kfree(ob);
}

static Object *enum_str(Object *ob, Object *arg)
{
  expect(type_isenum(OB_TYPE(ob)));
  EnumObject *eob = (EnumObject *)ob;
  Object *valstr = tuple_str(eob->values, NULL);
  Object *str;
  STRBUF(sbuf);
  strbuf_append(&sbuf, eob->name);
  strbuf_append(&sbuf, string_asstr(valstr));
  str = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  OB_DECREF(valstr);
  return str;
}

static Object *enum_equal(Object *ob1, Object *ob2)
{
  expect(type_isenum(OB_TYPE(ob1)));
  expect(type_isenum(OB_TYPE(ob2)));
  expect(OB_TYPE(ob1) == OB_TYPE(ob2));
  EnumObject *eob1 = (EnumObject *)ob1;
  EnumObject *eob2 = (EnumObject *)ob2;
  if (eob1 == eob2)
    return bool_true();
  if (strcmp(eob1->name, eob2->name))
    return bool_false();

  // check associated values
  Object *values1 = eob1->values;
  Object *values2 = eob2->values;
  if (values1 == NULL && values2 == NULL)
    return bool_true();

  if (tuple_size(values1) != tuple_size(values2))
    return bool_false();

  Object *item1;
  Object *item2;
  tuple_for_each(item1, values1) {
    item2 = tuple_get(values2, idx);
    if (OB_TYPE(item1) != OB_TYPE(item2))
      return bool_false();
  }

  return bool_true();
}

Object *enum_match(Object *patt, Object *some)
{
  expect(type_isenum(OB_TYPE(patt)));
  expect(type_isenum(OB_TYPE(some)));
  expect(OB_TYPE(patt) == OB_TYPE(some));
  EnumObject *eob1 = (EnumObject *)patt;
  EnumObject *eob2 = (EnumObject *)some;
  if (eob1 == eob2)
    return bool_true();
  if (strcmp(eob1->name, eob2->name))
    return bool_false();

  // check associated values
  Object *values1 = eob1->values;
  Object *values2 = eob2->values;
  if (values1 == NULL && values2 == NULL)
    return bool_true();

  expect(tuple_size(values1) == tuple_size(values2));
  return tuple_match(values1, values2);
}

static Object *enum_getitem(Object *self, Object *args)
{
  if (!integer_check(args)) {
    error("object of '%.64s' is not an Integer", OB_TYPE_NAME(args));
    return NULL;
  }

  int index = (int)integer_asint(args);
  return enum_get_value(self, index);
}

static MethodDef enum_methods[] = {
  {"__eq__", "A", "z", enum_equal},
  {"__getitem__", "i", "A", enum_getitem},
  {NULL},
};

TypeObject *enum_type_new(char *path, char *name)
{
  TypeObject *tp = gcnew(sizeof(TypeObject));
  init_object_head(tp, &type_type);
  tp->name  = atom(name);
  tp->desc  = desc_from_klass(path, name);
  tp->flags = TPFLAGS_ENUM;
  tp->free  = enum_free;
  tp->str   = enum_str;
  tp->equal = enum_equal;
  tp->methods = enum_methods;
  return tp;
}

static Object *enum_new_values(TypeObject *type, char *name, Object *values)
{
  EnumObject *eob = kmalloc(sizeof(EnumObject));
  init_object_head(eob, type);
  eob->name = atom(name);
  eob->values = OB_INCREF(values);
  return (Object *)eob;
}

Object *enum_new(Object *ob, char *name, Object *values)
{
  if (type_isenum(ob)) {
    return enum_new_values((TypeObject *)ob, name, values);
  } else {
    expect(type_isenum(OB_TYPE(ob)));
    EnumObject *eob = (EnumObject *)ob;
    if (values == NULL && !strcmp(eob->name, name)) {
      // return itself
      return OB_INCREF(ob);
    } else {
      return enum_new_values(OB_TYPE(ob), name, values);
    }
  }
}

int enum_check_byname(Object *ob, Object *name)
{
  expect(type_isenum(OB_TYPE(ob)));
  EnumObject *eob = (EnumObject *)ob;
  return !strcmp(eob->name, string_asstr(name));
}

int enum_check_value(Object *ob, Object *idx, Object *val)
{
  expect(type_isenum(OB_TYPE(ob)));
  EnumObject *eob = (EnumObject *)ob;
  int i = (int)integer_asint(idx);
  Object *val2 = tuple_get(eob->values, i);
  func_t fn = OB_NUM_FUNC(val, eq);
  Object *bval;
  if (fn != NULL) {
    bval = fn(val, val2);
  } else {
    bval = object_call(val, "__eq__", val2);
  }
  OB_DECREF(val2);
  int res = bool_istrue(bval);
  OB_DECREF(bval);
  return res;
}

Object *enum_get_value(Object *ob, int index)
{
  expect(type_isenum(OB_TYPE(ob)));
  EnumObject *eob = (EnumObject *)ob;
  expect(index >= 0 && index <= 31);
  expect(eob->values != NULL);
  return tuple_get(eob->values, index);
}
