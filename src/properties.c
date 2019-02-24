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

LOGGER(0)

typedef struct prop_entry {
  HashNode hnode;
  char *key;
  Vector vec;
} PropEntry;

static PropEntry *propentry_new(char *key)
{
  PropEntry *e = Malloc(sizeof(PropEntry));
  Init_HashNode(&e->hnode, e);
  e->key = AtomString_New(key).str;
  Vector_Init(&e->vec);
  return e;
}

static void propentry_free(PropEntry *e)
{
  Mfree(e);
}

static uint32 pe_hash(PropEntry *k)
{
  return hash_string(k->key);
}

static int pe_equal(PropEntry *k1, PropEntry *k2)
{
  return !strcmp(k1->key, k2->key);
}

int Properties_Init(Properties *prop)
{
  HashTable_Init(&prop->table, (hashfunc)pe_hash, (equalfunc)pe_equal);
  return 0;
}

static void __propentry_fini_func(HashNode *hnode, void *arg)
{
  PropEntry *e = container_of(hnode, PropEntry, hnode);
  Vector_Fini_Self(&e->vec);
  propentry_free(e);
}

void Properties_Fini(Properties *prop)
{
  HashTable_Fini(&prop->table, __propentry_fini_func, NULL);
}

int Properties_Put(Properties *prop, char *key, char *val)
{
  PropEntry k = {.key = key};
  HashNode *hnode = HashTable_Find(&prop->table, &k);
  PropEntry *entry = hnode ? container_of(hnode, PropEntry, hnode) : NULL;
  if (!entry) {
    entry = propentry_new(key);
    assert(entry != NULL);
    HashTable_Insert(&prop->table, &entry->hnode);
  }
  Vector_Append(&entry->vec, AtomString_New(val).str);
  return 0;
}

char *Properties_GetOne(Properties *prop, char *key)
{
  PropEntry k = {.key = key};
  HashNode *hnode = HashTable_Find(&prop->table, &k);
  PropEntry *e = hnode ? container_of(hnode, PropEntry, hnode) : NULL;
  if (e == NULL)
    return NULL;
  return Vector_Get(&e->vec, 0);
}

Vector *Properties_Get(Properties *prop, char *key)
{
  PropEntry k = {.key = key};
  HashNode *hnode = HashTable_Find(&prop->table, &k);
  PropEntry *e = hnode ? container_of(hnode, PropEntry, hnode) : NULL;
  return e != NULL ? &e->vec : NULL;
}
