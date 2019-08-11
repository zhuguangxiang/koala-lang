/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <string.h>
#include "hashmap.h"
#include "log.h"

#define FNV32_BASE  ((unsigned int) 0x811c9dc5)
#define FNV32_PRIME ((unsigned int) 0x01000193)

unsigned int strhash(const char *str)
{
	unsigned int c, hash = FNV32_BASE;
	while ((c = (unsigned char) *str++))
		hash = (hash * FNV32_PRIME) ^ c;
	return hash;
}

unsigned int memhash(const void *buf, size_t len)
{
	unsigned int hash = FNV32_BASE;
	unsigned char *ucbuf = (unsigned char *) buf;
	while (len--) {
		unsigned int c = *ucbuf++;
		hash = (hash * FNV32_PRIME) ^ c;
	}
	return hash;
}

#define HASHMAP_INITIAL_SIZE 32
#define HASHMAP_LOAD_FACTOR  80

static void alloc_entries(HashMap *self, int size)
{
  self->size = size;
  self->entries = kmalloc(size * sizeof(HashMapEntry *));
  /* calculate new thresholds */
  self->grow_at = size * HASHMAP_LOAD_FACTOR / 100;
  if (size <= HASHMAP_INITIAL_SIZE)
    self->shrink_at = 0;
  else
    self->shrink_at = self->grow_at / 5;
}

void hashmap_init(HashMap *self, equalfunc equalfunc)
{
  memset(self, 0, sizeof(*self));
  self->equalfunc = equalfunc;
  int size = HASHMAP_INITIAL_SIZE;
  alloc_entries(self, size);
}

void hashmap_fini(HashMap *self, freefunc freefunc, void *data)
{
  if (!self || !self->entries)
    return;

  if (freefunc) {
    HASHMAP_ITERATOR(iter, self);
    HashMapEntry *e;
    iter_for_each(&iter, e)
      freefunc(e, data);
  }

  kfree(self->entries);
  memset(self, 0, sizeof(*self));
}

static inline int bucket(HashMap *self, HashMapEntry *e)
{
  return e->hash & (self->size - 1);
}

static inline
int entry_equals(HashMap *self, HashMapEntry *e1, HashMapEntry *e2)
{
  return (e1 == e2) || (e1->hash == e2->hash && self->equalfunc(e1, e2));
}

static inline HashMapEntry **find_entry(HashMap *self, HashMapEntry *key)
{
  panic(!key->hash, "hash must not be 0");
	HashMapEntry **e = &self->entries[bucket(self, key)];
	while (*e && !entry_equals(self, *e, key))
		e = &(*e)->next;
	return e;
}

void *hashmap_get(HashMap *self, void *key)
{
  if (self == NULL)
    return NULL;
  return *find_entry(self, key);
}

static void rehash(HashMap *self, int newsize)
{
  int oldsize = self->size;
  HashMapEntry **oldentries = self->entries;

  alloc_entries(self, newsize);

  HashMapEntry *e;
  HashMapEntry *n;
  int b;
  for (int i = 0; i < oldsize; ++i) {
    e = oldentries[i];
    while (e) {
      n = e->next;
      b = bucket(self, e);
      e->next = self->entries[b];
      self->entries[b] = e;
      e = n;
    }
  }

  kfree(oldentries);
}

int hashmap_add(HashMap *self, void *entry)
{
  if (self == NULL)
    return -1;

  if (*find_entry(self, entry))
    return -1;

  int b = bucket(self, entry);
  ((HashMapEntry *)entry)->next = self->entries[b];
  self->entries[b] = entry;
  self->count++;
  if (self->count > self->grow_at)
    rehash(self, self->size << 1);
  return 0;
}

void *hashmap_put(HashMap *self, void *entry)
{
  HashMapEntry *old = hashmap_remove(self, entry);
  hashmap_add(self, entry);
  return old;
}

void *hashmap_remove(HashMap *self, void *key)
{
  HashMapEntry **e = find_entry(self, key);
  if (!*e)
    return NULL;

  HashMapEntry *old;
  old = *e;
  *e = old->next;
  old->next = NULL;

  self->count--;
  if (self->count < self->shrink_at)
    rehash(self, self->size >> 1);

  return old;
}

void *hashmap_iter_next(struct iterator *iter)
{
  HashMap *map = iter->iterable;
  if (map == NULL)
    return NULL;

  HashMapEntry *current = iter->item;
  for (;;) {
    if (current) {
      iter->item = current->next;
      return current;
    }

    if (iter->index >= map->size)
      return NULL;

    current = map->entries[iter->index++];
  }
}
