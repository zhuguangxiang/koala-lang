/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/*
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
 *   hashmap_entry_init(&s->entry, str_hash(s->value), s, s);
 *   hashmap_put(&map, &s->entry);
 * }
 */

#ifndef _KOALA_HASHMAP_H_
#define _KOALA_HASHMAP_H_

#include "common.h"
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*HashMapEqualFunc)(void *, void *);
typedef void (*HashMapVisitFunc)(void *, void *);

/* a hashmap entry is in the hashmap. */
typedef struct _HashMapEntry {
    /*
     * pointer to the next entry in collision list,
     * if multiple entries map to the same slots.
     */
    struct _HashMapEntry *next;
    /* entry's hash code */
    unsigned int hash;
} HashMapEntry;

/* a hashmap structure. */
typedef struct _HashMap {
    /* collision list array */
    HashMapEntry **entries;
    /* entries array size */
    int size;
    /* equal callback */
    HashMapEqualFunc equal;
    /* total number of entries */
    int count;
    /* expand entries point */
    int grow_at;
    /* shrink entries point */
    int shrink_at;
} HashMap;

/* Initialize a HashMapEntry structure. */
static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
    HashMapEntry *e = (HashMapEntry *)entry;
    e->hash = hash;
    e->next = null;
}

/* Return the number of items in the map. */
static inline int hashmap_size(HashMap *self)
{
    return self ? self->count : 0;
}

/* Initialize a hash map. */
void hashmap_init(HashMap *self, HashMapEqualFunc equal);

/* Destroy the hashmap and free its allocated memory.
 * NOTES: HashMapEntry is already removed, no remove it again.
 */
void hashmap_fini(HashMap *self, HashMapVisitFunc free, void *arg);

/*
 * Retrieve the hashmap entry for the specified hash code.
 * Returns the hashmap entry or null if not found.
 */
void *hashmap_get(HashMap *self, void *key);

/*
 * Add a hashmap entry. No check duplication.
 */
void hashmap_put_only(HashMap *self, void *entry);

/*
 * Add a hashmap entry. If the hashmap contains duplicate entries, it will
 * return -1 failure.
 */
int hashmap_put_absent(HashMap *self, void *entry);

/*
 * Add or replace a hashmap entry. If the hashmap contains duplicate entries,
 * the old entry will be replaced and returned.
 * Returns the replaced entry or null if no duplicated entry
 */
void *hashmap_put(HashMap *self, void *entry);

/* Removes a hashmap entry matching the specified key. */
void *hashmap_remove(HashMap *self, void *key);

/* Visit the hashmap, safely when `visit` removes entry, not suggest do it. */
void hashmap_visit(HashMap *self, HashMapVisitFunc visit, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHMAP_H_ */
