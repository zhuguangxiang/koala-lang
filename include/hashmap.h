/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
unsigned int memhash(const void *buf, int len);

typedef int (*hashmap_equal_func)(void *, void *);
typedef void (*hashmap_visit_func)(void *, void *);

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
    hashmap_equal_func equal;
    /* total number of entries */
    int count;
    /* expand entries point */
    int grow_at;
    /* shrink entries point */
    int shrink_at;
} HashMap, *HashMapRef;

/* Initialize a HashMapEntry structure. */
static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
    HashMapEntry *e = (HashMapEntry *)entry;
    e->hash = hash;
    e->next = NULL;
}

/* Return the number of items in the map. */
static inline int hashmap_size(HashMapRef self)
{
    return self ? self->count : 0;
}

/* Initialize a hash map. */
void hashmap_init(HashMapRef self, hashmap_equal_func equal);

/* Destroy the hashmap and free its allocated memory.
 * NOTES: HashMapEntry is already removed, no remove it again.
 */
void hashmap_fini(HashMapRef self, hashmap_visit_func free, void *arg);

/*
 * Retrieve the hashmap entry for the specified hash code.
 * Returns the hashmap entry or null if not found.
 */
void *hashmap_get(HashMapRef self, void *key);

/*
 * Add a hashmap entry. If the hashmap contains duplicate entries, it will
 * return -1 failure.
 */
int hashmap_put_absent(HashMapRef self, void *entry);

/*
 * Add or replace a hashmap entry. If the hashmap contains duplicate entries,
 * the old entry will be replaced and returned.
 * Returns the replaced entry or null if no duplicated entry
 */
void *hashmap_put(HashMapRef self, void *entry);

/* Removes a hashmap entry matching the specified key. */
void *hashmap_remove(HashMapRef self, void *key);

/* Visit the hashmap, safely when `visit` removes entry, not suggest do it. */
void hashmap_visit(HashMapRef self, hashmap_visit_func visit, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHMAP_H_ */
