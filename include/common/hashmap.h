/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
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
 *   hashmap_entry_init(&s->entry, str_hash(s->value), s, s);
 *   hashmap_put(&map, &s->entry);
 * }
 */

#ifndef _KOALA_HASHMAP_H_
#define _KOALA_HASHMAP_H_

#include "common.h"
#include "hlist.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ready-to-use hash functions for strings, using the FNV-1 algorithm.
 * (see http://www.isthe.com/chongo/tech/comp/fnv).
 * `str_hash` takes 0-terminated strings.
 * `mem_hash` operates on arbitrary-length memory.
 */

unsigned int str_hash(const char *buf);
unsigned int mem_hash(const void *buf, int len);

typedef int (*HashMapEqualFunc)(void *, void *);
typedef void (*HashMapVisitFunc)(void *, void *);

/* a hashmap entry is in the hashmap. */
typedef struct _HashMapEntry {
    /* conflict list */
    HListNode hnode;
    /* entry's hash code */
    unsigned int hash;
    /* ordered list */
    List ord_node;
} HashMapEntry;

/* a hashmap structure. */
typedef struct _HashMap {
    /* collision list array */
    HListHead *entries;
    /* ordered list */
    List ord_list;
    /* equal callback */
    HashMapEqualFunc equal;
    /* entries array size */
    int size;
    /* total number of entries */
    int count;
    /* expand entries point */
    int grow_at;
    /* shrink entries point */
    int shrink_at;
} HashMap;

/* Hashmap iterator context */
typedef struct _HashMapIter {
    /* internal state */
    int state;
    /* current node */
    HashMapEntry *entry;
    /* next node */
    HashMapEntry *next;
    /* closure */
    void *arg;
} HashMapIter;

/* next() iterator, 1 continue, 0 finished */
int hashmap_next(HashMap *self, HashMapIter *it);

/* prev() iterator, 1 continue, 0 finished */
int hashmap_prev(HashMap *self, HashMapIter *it);

/* get entry from iterator */
#define hashmap_entry(it) ((it)->entry)

/* Initialize a HashMapEntry structure. */
static inline void hashmap_entry_init(void *entry, unsigned int hash)
{
    HashMapEntry *e = (HashMapEntry *)entry;
    e->hash = hash;
    init_hlist_node(&e->hnode);
    init_list(&e->ord_node);
}

/* Return the number of items in the map. */
static inline int hashmap_size(HashMap *self) { return self ? self->count : 0; }

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

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHMAP_H_ */
