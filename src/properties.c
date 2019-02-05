/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "properties.h"
#include "hashfunc.h"
#include "mem.h"
#include "log.h"

static Logger logger;

typedef struct prop_entry {
  HashNode hnode;
  String key;
  int count;
  struct list_head list;
} PropEntry;

static PropEntry *propentry_new(char *key)
{
  PropEntry *e = mm_alloc(sizeof(PropEntry));
  Init_HashNode(&e->hnode, e);
  e->key = AtomString_New(key);
  init_list_head(&e->list);
  return e;
}

static void propentry_free(PropEntry *e)
{
  mm_free(e);
}

static uint32 pe_hash(PropEntry *k)
{
  return hash_string(k->key.str);
}

static int pe_equal(PropEntry *k1, PropEntry *k2)
{
  return !strcmp(k1->key.str, k2->key.str);
}

int Properties_Init(Properties *prop)
{
  HashTable_Init(&prop->table, (hashfunc)pe_hash, (equalfunc)pe_equal);
  return 0;
}

static Property *property_new(char *val)
{
  Property *p = mm_alloc(sizeof(Property));
  init_list_head(&p->link);
  p->val = AtomString_New(val);
  return p;
}

static void property_free(Property *property)
{
  mm_free(property);
}

static void __propentry_fini_func(HashNode *hnode, void *arg)
{
  PropEntry *e = container_of(hnode, PropEntry, hnode);
  Property *property;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &e->list) {
    list_del(p);
    property = container_of(p, Property, link);
    property_free(property);
  }
  propentry_free(e);
}

void Properties_Fini(Properties *prop)
{
  HashTable_Fini(&prop->table, __propentry_fini_func, NULL);
}

static void add_property(PropEntry *e, char *val)
{
  Property *p = property_new(val);
  list_add_tail(&p->link, &e->list);
  e->count++;
}

int Properties_Put(Properties *prop, char *key, char *val)
{
  PropEntry k = {.key = key};
  HashNode *hnode = HashTable_Find(&prop->table, &k);
  PropEntry *entry = hnode ? container_of(hnode, PropEntry, hnode) : NULL;
  if (!entry) {
    entry = propentry_new(key);
    assert(entry);
    HashTable_Insert(&prop->table, &entry->hnode);
  }
  add_property(entry, val);
  return 0;
}

char *Properties_Get(Properties *prop, char *key)
{
  PropEntry k = {.key = key};
  HashNode *hnode = HashTable_Find(&prop->table, &k);
  PropEntry *e = hnode ? container_of(hnode, PropEntry, hnode) : NULL;
  if (!e)
    return NULL;
  Property *p = container_of(list_first(&e->list), Property, link);
  return Property_Value(p);
}

struct list_head *Properties_Get_List(Properties *prop, char *key)
{
  PropEntry k = {.key = key};
  HashNode *hnode = HashTable_Find(&prop->table, &k);
  PropEntry *e = hnode ? container_of(hnode, PropEntry, hnode) : NULL;
  return e ? &e->list : NULL;
}
