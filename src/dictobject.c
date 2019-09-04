/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "dictobject.h"
#include "intobject.h"
#include "tupleobject.h"

typedef struct dictentry {
  hashmapentry entry;
  Object *key;
  Object *val;
} DictEntry;

static int dict_entry_equal(void *e1, void *e2)
{
  DictEntry *de1 = e1;
  DictEntry *de2 = e2;
  if (de1 == de2)
    return 1;
  return Object_Equal(de1->key, de2->key);
}

static void dict_entry_free(void *e, void *arg)
{
  DictEntry *de = e;
  OB_DECREF(de->key);
  OB_DECREF(de->val);
  kfree(e);
}

Object *Dict_New_Types(TypeObject *ktype, TypeObject *vtype)
{
  DictObject *dict = kmalloc(sizeof(*dict));
  Init_Object_Head(dict, &Dict_Type);
  dict->ktype = OB_INCREF(ktype);
  dict->vtype = OB_INCREF(vtype);
  hashmap_init(&dict->map, dict_entry_equal);
  return (Object *)dict;
}

Object *Dict_Get(Object *self, Object *key)
{
  if (!Dict_Check(self)) {
    error("object of '%.64s' is not a Dict", OB_TYPE_NAME(self));
    return NULL;
  }

  DictObject *dict = (DictObject *)self;

  if (dict->ktype != OB_TYPE(key)) {
    error("key of '%.64s' is not matched with '%.64s'",
          OB_TYPE_NAME(key), dict->ktype->name);
    return NULL;
  }

  unsigned int hash = Object_Hash(key);
  DictEntry entry = {.key = key};
  hashmap_entry_init(&entry, hash);
  DictEntry *val = hashmap_get(&dict->map, &entry);
  return val ? OB_INCREF(val->val) : NULL;
}

int Dict_Put(Object *self, Object *key, Object *val)
{
  if (!Dict_Check(self)) {
    error("object of '%.64s' is not a Dict", OB_TYPE_NAME(self));
    return -1;
  }

  DictObject *dict = (DictObject *)self;

  if (dict->ktype != OB_TYPE(key)) {
    error("key of '%.64s' is not matched with '%.64s'",
          OB_TYPE_NAME(key), dict->ktype->name);
    return -1;
  }

  if (dict->vtype != &Any_Type && OB_TYPE(val) != dict->vtype) {
    error("value of '%s64.s' is not matched with '%.64s'",
          OB_TYPE_NAME(val), dict->vtype->name);
    return -1;
  }

  unsigned int hash = Object_Hash(key);
  DictEntry *entry = kmalloc(sizeof(*entry));
  hashmap_entry_init(entry, hash);
  entry->key = OB_INCREF(key);
  entry->val = OB_INCREF(val);
  return hashmap_add(&dict->map, entry);
}

static Object *dict_setitem(Object *self, Object *args)
{
  if (!Tuple_Check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return Bool_False();
  }

  Object *key = Tuple_Get(args, 0);
  Object *val = Tuple_Get(args, 1);
  int res = Dict_Put(self, key, val);
  OB_DECREF(key);
  OB_DECREF(val);
  return (res < 0) ? Bool_False() : Bool_True();
}

static void dict_free(Object *ob)
{
  if (!Dict_Check(ob)) {
    error("object of '%.64s' is not a Dict", OB_TYPE_NAME(ob));
    return;
  }

  DictObject *dict = (DictObject *)ob;
  OB_DECREF(dict->ktype);
  OB_DECREF(dict->vtype);
  hashmap_fini(&dict->map, dict_entry_free, NULL);
  kfree(ob);
}

static MethodDef dict_methods[] = {
  {"__getitem__", "A",  "A",  Dict_Get    },
  {"__setitem__", "AA", NULL, dict_setitem},
  {NULL}
};

TypeObject Dict_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Dict",
  .free    = dict_free,
  .methods = dict_methods,
};
