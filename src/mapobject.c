/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "mapobject.h"
#include "intobject.h"
#include "tupleobject.h"

typedef struct mapentry {
  struct hashmap_entry entry;
  Object *key;
  Object *val;
} MapEntry;

static int map_entry_equal(void *e1, void *e2)
{
  MapEntry *me1 = e1;
  MapEntry *me2 = e2;
  if (me1 == me2)
    return 1;
  return Object_Equal(me1->key, me2->key);
}

Object *Map_New_Types(TypeObject *ktype, TypeObject *vtype)
{
  MapObject *map = kmalloc(sizeof(*map));
  Init_Object_Head(map, &Map_Type);
  map->ktype = OB_INCREF(ktype);
  map->vtype = OB_INCREF(vtype);
  hashmap_init(&map->map, map_entry_equal);
  return (Object *)map;
}

Object *Map_Get(Object *self, Object *key)
{
  if (!Map_Check(self)) {
    error("object of '%.64s' is not a Map", OB_TYPE_NAME(self));
    return NULL;
  }

  MapObject *map = (MapObject *)self;

  if (!Type_Equal(map->ktype, OB_TYPE(key))) {
    error("key of '%.64s' is not matched with '%.64s'",
          OB_TYPE_NAME(key), map->ktype->name);
    return NULL;
  }

  unsigned int hash;
  if (Object_Hash(key, &hash) < 0)
    return NULL;

  MapEntry entry = { .key = key };
  hashmap_entry_init(&entry, hash);
  MapEntry *val = hashmap_get(&map->map, &entry);
  return val ? val->val : NULL;
}

int Map_Put(Object *self, Object *key, Object *val)
{
  if (!Map_Check(self)) {
    error("object of '%.64s' is not a Map", OB_TYPE_NAME(self));
    return -1;
  }

  MapObject *map = (MapObject *)self;

  if (!Type_Equal(map->ktype, OB_TYPE(key))) {
    error("key of '%.64s' is not matched with '%.64s'",
          OB_TYPE_NAME(key), map->ktype->name);
    return -1;
  }

  if (!Type_Equal(map->vtype, &Any_Type)
      && !Type_Equal(OB_TYPE(val), map->vtype)) {
    error("value of '%s64.s' is not matched with '%.64s'",
          OB_TYPE_NAME(val), map->vtype->name);
    return -1;
  }

  unsigned int hash;
  if (Object_Hash(key, &hash) < 0)
    return -1;

  MapEntry *entry = kmalloc(sizeof(*entry));
  hashmap_entry_init(entry, hash);
  entry->key = OB_INCREF(key);
  entry->val = OB_INCREF(val);
  return hashmap_add(&map->map, entry);
}

static Object *map_setitem(Object *self, Object *args)
{
  if (!Tuple_Check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return Bool_False();
  }

  Object *key = Tuple_Get(args, 0);
  Object *val = Tuple_Get(args, 1);
  int res = Map_Put(self, key, val);
  OB_DECREF(key);
  OB_DECREF(val);
  return (res < 0) ? Bool_False() : Bool_True();
}

static MappingMethods map_mapping = {
  .getitem = Map_Get,
  .setitem = map_setitem,
};

TypeObject Map_Type = {
  OBJECT_HEAD_INIT(&Type_Type)
  .name    = "Map",
  .lookup  = Object_Lookup,
  .mapping = &map_mapping,
};
