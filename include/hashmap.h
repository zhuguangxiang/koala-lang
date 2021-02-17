/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

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
 *   hashmap_entry_init(&s->entry, strhash(s->value), s, s);
 *   hashmap_put(&map, &s->entry);
 * }
 */

#ifndef _KOALA_HASHMAP_H_
#define _KOALA_HASHMAP_H_

#include <stddef.h>

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
unsigned int memhash(const void *buf, int len);

typedef int (*hashmap_equal_t)(void *, void *);
typedef void (*hashmap_visit_t)(void *, void *);

typedef struct hashmap_entry hashmap_entry_t;

/* a hashmap entry is in the hashmap. */
struct hashmap_entry {
    /*
     * pointer to the next entry in collision list,
     * if multiple entries map to the same slots.
     */
    hashmap_entry_t *next;
    /* entry's hash code */
    unsigned int hash;
};

/* a hashmap structure. */
typedef struct hashmap {
    /* collision list array */
    hashmap_entry_t **entries;
    /* entries array size */
    int size;
    /* equal callback */
    hashmap_equal_t equal;
    /* total number of entries */
    int count;
    /* expand entries point */
    int grow_at;
    /* shrink entries point */
    int shrink_at;
} hashmap_t;

/* Initialize a HashMapEntry structure. */
static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
    hashmap_entry_t *e = (hashmap_entry_t *)entry;
    e->hash = hash;
    e->next = NULL;
}

/* Return the number of items in the map. */
static inline int hashmap_size(hashmap_t *self)
{
    return self ? self->count : 0;
}

/* Initialize a hash map. */
void hashmap_init(hashmap_t *self, hashmap_equal_t equal);

/* Destroy the hashmap and free its allocated memory.
 * NOTES: HashMapEntry is already removed, no remove it again.
 */
void hashmap_fini(hashmap_t *self, hashmap_visit_t free, void *arg);

/*
 * Retrieve the hashmap entry for the specified hash code.
 * Returns the hashmap entry or null if not found.
 */
void *hashmap_get(hashmap_t *self, void *key);

/*
 * Add a hashmap entry. If the hashmap contains duplicate entries, it will
 * return -1 failure.
 */
int hashmap_put_absent(hashmap_t *self, void *entry);

/*
 * Add or replace a hashmap entry. If the hashmap contains duplicate entries,
 * the old entry will be replaced and returned.
 * Returns the replaced entry or null if no duplicated entry
 */
void *hashmap_put(hashmap_t *self, void *entry);

/* Removes a hashmap entry matching the specified key. */
void *hashmap_remove(hashmap_t *self, void *key);

/* Visit the hashmap, safely when `visit` removes entry, not suggest do it. */
void hashmap_visit(hashmap_t *self, hashmap_visit_t visit, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHMAP_H_ */
