/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Generic implementation of hash-based key-value mappings.
 *
 * An example of using hashmap as hashset.
 *
 * struct string {
 *   struct hashmap_entry entry;
 *   int length;
 *   char *value;
 * };
 *
 * struct hashmap map;
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
struct hashmap_entry {
  /*
   * pointer to the next entry in collision list,
   * if multiple entries map to the same slots.
   */
  struct hashmap_entry *next;
  /* entry's hash code */
  unsigned int hash;
};

/*
 * Compare function to test two keys for equality.
 * Returns 0 if the twoe entries are equal.
 */
typedef int (*hashmap_cmpfunc)(void *k1, void *k2);

/* a hash map structure. */
struct hashmap {
  /* collision list array */
  struct hashmap_entry **entries;
  /* entries array size */
  int size;
  /* comparison function */
  hashmap_cmpfunc cmpfunc;
  /* total number of entries */
  int count;
  /* expand entries array point */
  int grow_at;
  /* shrink entries array point */
  int shrink_at;
};

/* Initialize a hashmap_entry structure. */
static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
  struct hashmap_entry *e = entry;
  e->hash = hash;
  e->next = NULL;
}

/* Return the number of items in the map. */
static inline int hashmap_size(struct hashmap *self)
{
  return self->count;
}

/* Initialize a hash map. */
void hashmap_init(struct hashmap *self, hashmap_cmpfunc cmpfunc);

/* Free function for hashmap entry, when the hashmap is destroyed. */
typedef void (*hashmap_freefunc)(void *entry, void *data);

/* Destroy the hashmap and free its allocated memory. */
void hashmap_free(struct hashmap *self, hashmap_freefunc freefunc, void *data);

/*
 * Retrieve the hashmap entry for the specified hash code.
 * Returns the hashmap entry or null if not found.
 */
void *hashmap_get(struct hashmap *self, void *key);

/*
 * Add a hashmap entry. If the hashmap contains duplicate entries, it will
 * return -1 failure.
 */
int hashmap_add(struct hashmap *self, void *entry);

/*
 * Add or replace a hashmap entry. If the hashmap contains duplicate entries,
 * the old entry will be replaced and returned.
 * Returns the replaced entry or null if no duplicated entry
 */
void *hashmap_put(struct hashmap *self, void *entry);

/* Removes a hashmap entry matching the specified key. */
void *hashmap_remove(struct hashmap *self, void *key);

/*
 * Iterator callback function for hashmap iteration.
 * See iterator.h.
 */
void *hashmap_iter_next(struct iterator *iter);

/* Declare an iterator of the hashmap. Deletion is not safe. */
#define HASHMAP_ITERATOR(name, hashmap) \
  ITERATOR(name, hashmap, hashmap_iter_next)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHMAP_H_ */
