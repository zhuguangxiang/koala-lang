/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "dictobject.h"
#include "intobject.h"

typedef struct Dictentry {
  struct hashmap_entry entry;
  Object *key;
  Object *val;
} DictEntry;

static int _dict_entry_cmp_cb_(void *e1, void *e2)
{
  DictEntry *entry1 = e1;
  DictEntry *entry2 = e2;
  if (e1 == e2)
    return 0;
  TypeObject *type1 = OB_TYPE(entry1->key);
  TypeObject *type2 = OB_TYPE(entry2->key);
  if (!Type_Equal(type1, type2))
    return -1;
  Object *ob = Call_Function(entry1->key, "__cmp__", entry2->key);
  return Integer_AsInt(ob);
}

Object *Dict_New_WithTypes(TypeObject *ktype, TypeObject *vtype)
{
  DictObject *dict = kmalloc(sizeof(*dict));
  Init_Object_Head(dict, &Dict_Type);
  dict->ktype = OB_INCREF(ktype);
  dict->vtype = OB_INCREF(vtype);
  hashmap_init(&dict->map, _dict_entry_cmp_cb_);
  return (Object *)dict;
}

Object *Dict_Get(Object *self, Object *key)
{
  if (!Dict_Check(self)) {
    error("object of '%.64s' is not a dict.", OB_TYPE(self)->name);
    return NULL;
  }

  DictObject *dict = (DictObject *)self;

  if (!Type_Equal(dict->ktype, OB_TYPE(key))) {
    error("key of '%.64s' is not matched with '%.64s'",
          OB_TYPE(key)->name, dict->ktype->name);
    return NULL;
  }

  Object *ob = Call_Function(key, "__hash__", NULL);
  unsigned int hash = Integer_AsInt(ob);
  if (!hash)
    return NULL;

  DictEntry entry = { .key = key };
  hashmap_entry_init(&entry, hash);
  DictEntry *val = hashmap_get(&dict->map, &entry);
  return val ? val->val : NULL;
}

int Dict_Put(Object *self, Object *key, Object *val)
{
  if (!Dict_Check(self)) {
    error("object of '%.64s' is not a dict.", OB_TYPE(self)->name);
    return 0;
  }

  DictObject *dict = (DictObject *)self;

  if (!Type_Equal(dict->ktype, OB_TYPE(key))) {
    error("key of '%.64s' is not matched with '%.64s'",
          OB_TYPE(key)->name, dict->ktype->name);
    return 0;
  }

  if (!Type_Equal(dict->vtype, &Any_Type)
      && !Type_Equal(OB_TYPE(val), dict->vtype)) {
    error("value of '%s64.s' is not matched with '%.64s'",
          OB_TYPE(val)->name, dict->vtype->name);
    return 0;
  }

  int res = Dict_Contains(self, key);
  if (res) {
    warn("key of '%.64s' exists.", OB_TYPE(key)->name);
    return 0;
  }

  Object *ob = Call_Function(key, "__hash__", NULL);
  unsigned int hash = Integer_AsInt(ob);
  if (!hash)
    return 0;

  DictEntry *entry = kmalloc(sizeof(*entry));
  hashmap_entry_init(entry, hash);
  entry->key = OB_INCREF(key);
  entry->val = OB_INCREF(val);
  return hashmap_add(&dict->map, entry);
}

static Object *_dict_setitem_cb_(Object *self, Object *args)
{

}

static MappingMethods dict_mapping = {
  .getitem = Dict_Get,
  .setitem = _dict_setitem_cb_,
};

TypeObject Dict_Type = {
  OBJECT_HEAD_INIT(&Class_Type)
  .name = "Dict",
  .mapping = &dict_mapping,
};
