/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Generic implementation of hash-based key-value mappings.
 *
 * An example of using hashmap as hashset.
 *
 * struct string {
 *   HashMapEntry entry;
 *   int length;
 *   char *value;
 * };
 *
 * HashMap map;
 * int string_compare(void *k1, void *k2)
 * {
 *   struct string *s1 = k1;
 *   struct string *s2 = k2;
 *   if (s1->length != s2->length || strcmp(s1->value, s2->value))
 *     return 0;
 *   else
 *     return 1;
 * }
 *
 * int main(int argc, char *argv[])
 * {
 *   hashmap_init(&map, string_compare);
 *   struct string *s = malloc(sizeof(*s));
 *   s->length = strlen("some string");
 *   s->value = "some string";
 *   hashmap_entry_init(&s->entry, strhash(s->value), s, s);
 *   hashmap_add(&map, &s->entry);
 * }
 */

#ifndef _KOALA_HASHMAP_H_
#define _KOALA_HASHMAP_H_

#include "memory.h"
#include "iterator.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ready-to-use hash functions for strings, using the FNV-1 algorithm.
 * (see http://www.isthe.com/chongo/tech/comp/fnv).
 * `strhash` takes 0-terminated strings.
 * `memhash` operates on arbitrary-length memory.
 */
unsigned int strhash(const char *buf);
unsigned int memhash(const void *buf, size_t len);

/* a hash map entry is in the hash table. */
typedef struct hashmapentry {
  /*
   * pointer to the next entry in collision list,
   * if multiple entries map to the same slots.
   */
  struct hashmapentry *next;
  /* entry's hash code */
  unsigned int hash;
} HashMapEntry;

/* a hash map structure. */
typedef struct hashmap {
  /* collision list array */
  HashMapEntry **entries;
  /* entries array size */
  int size;
  /* equal function */
  equalfunc equalfunc;
  /* total number of entries */
  int count;
  /* expand entries array point */
  int grow_at;
  /* shrink entries array point */
  int shrink_at;
} HashMap;

/* Initialize a hashmap_entry structure. */
static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
  HashMapEntry *e = entry;
  e->hash = hash;
  e->next = NULL;
}

/* Return the number of items in the map. */
static inline int hashmap_size(HashMap *self)
{
  return self ? self->count : 0;
}

/* Initialize a hash map. */
void hashmap_init(HashMap *self, equalfunc equalfunc);

/* Destroy the hashmap and free its allocated memory. */
void hashmap_fini(HashMap *self, freefunc freefunc, void *data);

/*
 * Retrieve the hashmap entry for the specified hash code.
 * Returns the hashmap entry or null if not found.
 */
void *hashmap_get(HashMap *self, void *key);

/*
 * Add a hashmap entry. If the hashmap contains duplicate entries, it will
 * return -1 failure.
 */
int hashmap_add(HashMap *self, void *entry);

/*
 * Add or replace a hashmap entry. If the hashmap contains duplicate entries,
 * the old entry will be replaced and returned.
 * Returns the replaced entry or null if no duplicated entry
 */
void *hashmap_put(HashMap *self, void *entry);

/* Removes a hashmap entry matching the specified key. */
void *hashmap_remove(HashMap *self, void *key);

/*
 * Iterator callback function for hashmap iteration.
 * See iterator.h.
 */
void *hashmap_iter_next(Iterator *iter);

/* Declare an iterator of the hashmap. Deletion is not safe. */
#define HASHMAP_ITERATOR(name, hashmap) \
  ITERATOR(name, hashmap, hashmap_iter_next)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHMAP_H_ */
