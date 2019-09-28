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

#include "mapobject.h"
#include "intobject.h"
#include "tupleobject.h"

typedef struct mapentry {
  HashMapEntry entry;
  Object *key;
  Object *val;
} MapEntry;

static int mapentry_equal(void *e1, void *e2)
{
  MapEntry *me1 = e1;
  MapEntry *me2 = e2;
  if (me1 == me2)
    return 1;
  return Object_Cmp(me1->key, me2->key);
}

static void mapentry_free(void *e, void *arg)
{
  MapEntry *me = e;
  OB_DECREF(me->key);
  OB_DECREF(me->val);
  kfree(e);
}

Object *map_new(TypeDesc *ktype, TypeDesc *vtype)
{
  MapObject *map = kmalloc(sizeof(*map));
  init_object_head(map, &map_type);
  map->ktype = TYPE_INCREF(ktype);
  map->vtype = TYPE_INCREF(vtype);
  hashmap_init(&map->map, mapentry_equal);
  return (Object *)map;
}

Object *map_get(Object *self, Object *key)
{
  if (!map_check(self)) {
    error("object of '%.64s' is not a Map", OB_TYPE_NAME(self));
    return NULL;
  }

  MapObject *map = (MapObject *)self;
  unsigned int hash = Object_Hash(key);
  MapEntry entry = {.key = OB_INCREF(key)};
  hashmap_entry_init(&entry, hash);
  MapEntry *val = hashmap_get(&map->map, &entry);
  OB_DECREF(key);
  return val ? OB_INCREF(val->val) : NULL;
}

int map_put(Object *self, Object *key, Object *val)
{
  if (!map_check(self)) {
    error("object of '%.64s' is not a Map", OB_TYPE_NAME(self));
    return -1;
  }

  MapObject *map = (MapObject *)self;
  unsigned int hash = Object_Hash(key);
  MapEntry *entry = kmalloc(sizeof(*entry));
  hashmap_entry_init(entry, hash);
  entry->key = OB_INCREF(key);
  entry->val = OB_INCREF(val);
  MapEntry *old = hashmap_put(&map->map, entry);
  if (old != NULL) {
    OB_DECREF(old->key);
    OB_DECREF(old->val);
    kfree(old);
  }
  return 0;
}

static Object *map_set(Object *self, Object *args)
{
  if (!Tuple_Check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return Bool_False();
  }

  Object *key = tuple_get(args, 0);
  Object *val = tuple_get(args, 1);
  int res = map_put(self, key, val);
  OB_DECREF(key);
  OB_DECREF(val);
  return (res < 0) ? Bool_False() : Bool_True();
}

static void map_free(Object *ob)
{
  if (!map_check(ob)) {
    error("object of '%.64s' is not a Map", OB_TYPE_NAME(ob));
    return;
  }

  MapObject *map = (MapObject *)ob;
  TYPE_DECREF(map->ktype);
  TYPE_DECREF(map->vtype);
  hashmap_fini(&map->map, mapentry_free, NULL);
  kfree(ob);
}

static void print_object(Object *ob, StrBuf *sbuf)
{
  if (string_check(ob)) {
    strbuf_append_char(sbuf, '"');
    strbuf_append(sbuf, string_asstr(ob));
    strbuf_append_char(sbuf, '"');
  } else {
    Object *str = Object_Call(ob, "__str__", NULL);
    strbuf_append(sbuf, string_asstr(str));
    OB_DECREF(str);
  }
}

Object *map_str(Object *self, Object *ob)
{
  if (!map_check(self)) {
    error("object of '%.64s' is not a Map", OB_TYPE_NAME(self));
    return NULL;
  }

  MapObject *map = (MapObject *)self;
  STRBUF(sbuf);
  HASHMAP_ITERATOR(iter, &map->map);
  strbuf_append_char(&sbuf, '{');
  Object *str;
  MapEntry *tmp;
  int i = 0;
  int size = hashmap_size(&map->map);
  iter_for_each(&iter, tmp) {
    print_object(tmp->key, &sbuf);
    strbuf_append_char(&sbuf, ':');
    print_object(tmp->val, &sbuf);
    if (i++ < size - 1)
      strbuf_append(&sbuf, ", ");
  }
  strbuf_append(&sbuf, "}");
  str = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);

  return str;
}

static MethodDef map_methods[] = {
  {"__getitem__", "<K>",    "<V>",  map_get},
  {"__setitem__", "<K><V>", NULL,   map_set},
  {NULL}
};

TypeObject map_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Map",
  .free    = map_free,
  .str     = map_str,
  .methods = map_methods,
};

void init_map_type(void)
{
  TypeDesc *desc = desc_from_map;
  TypeDesc *para = desc_from_paradef("K", NULL);
  desc_add_paradef(desc, para);
  TYPE_DECREF(para);
  para = desc_from_paradef("V", NULL);
  desc_add_paradef(desc, para);
  TYPE_DECREF(para);
  map_type.desc = desc;
  if (type_ready(&map_type) < 0)
    panic("Cannot initalize 'Map' type.");
}
