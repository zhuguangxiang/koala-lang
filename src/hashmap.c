/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "hashmap.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FNV32_BASE  ((unsigned int)0x811c9dc5)
#define FNV32_PRIME ((unsigned int)0x01000193)

unsigned int strhash(const char *str)
{
    unsigned int c, hash = FNV32_BASE;
    while ((c = (unsigned char)*str++)) hash = (hash * FNV32_PRIME) ^ c;
    return hash;
}

unsigned int memhash(const void *buf, int len)
{
    unsigned int hash = FNV32_BASE;
    unsigned char *ucbuf = (unsigned char *)buf;
    while (len--) {
        unsigned int c = *ucbuf++;
        hash = (hash * FNV32_PRIME) ^ c;
    }
    return hash;
}

#define HASHMAP_INITIAL_SIZE 32
#define HASHMAP_LOAD_FACTOR  65

static void __alloc_entries(HashMap *self, int size)
{
    self->size = size;
    self->entries = mm_alloc(size * sizeof(HashMapEntry *));
    /* calculate new thresholds */
    self->grow_at = size * HASHMAP_LOAD_FACTOR / 100;
    if (size <= HASHMAP_INITIAL_SIZE)
        self->shrink_at = 0;
    else
        self->shrink_at = self->grow_at / 5;
    /*
    printf("hashmap:\nentries: %d, grow_at: %d, shrink_at: %d\n", self->size,
        self->grow_at, self->shrink_at);
    */
}

void hashmap_init(HashMap *self, hash_equal_t equal)
{
    memset(self, 0, sizeof(HashMap));
    self->equal = equal;
    __alloc_entries(self, HASHMAP_INITIAL_SIZE);
}

void hashmap_fini(HashMap *self, hash_visit_t free, void *arg)
{
    if (!self || !self->entries) return;

    HashMapEntry *e, *nxt;
    for (int i = 0; i < self->size; i++) {
        e = self->entries[i];
        while (e) {
            nxt = e->next;
            free(e, arg);
            e = nxt;
        }
    }

    mm_free(self->entries);
}

static inline int bucket(HashMap *self, HashMapEntry *e)
{
    return e->hash & (self->size - 1);
}

static inline int entry_equals(
    HashMap *self, HashMapEntry *e1, HashMapEntry *e2)
{
    return (e1 == e2) || (e1->hash == e2->hash && self->equal(e1, e2));
}

static inline HashMapEntry **find_entry(HashMap *self, HashMapEntry *key)
{
    HashMapEntry **e = &self->entries[bucket(self, key)];
    while (*e && !entry_equals(self, *e, key)) e = &(*e)->next;
    return e;
}

void *hashmap_get(HashMap *self, void *key)
{
    if (!self) return NULL;
    return *find_entry(self, key);
}

static void rehash(HashMap *self, int newsize)
{
    int oldsize = self->size;
    printf("hashmap:\nrehashing: %d\n", self->count);
    HashMapEntry **oldentries = self->entries;

    __alloc_entries(self, newsize);

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

    mm_free(oldentries);
}

int hashmap_put_absent(HashMap *self, void *entry)
{
    if (self == NULL) return -1;

    HashMapEntry *e = entry;

    if (*find_entry(self, e)) return -1;

    int b = bucket(self, e);
    e->next = self->entries[b];
    self->entries[b] = e;
    self->count++;
    if (self->count > self->grow_at) rehash(self, self->size << 1);
    return 0;
}

void *hashmap_put(HashMap *self, void *entry)
{
    void *old = hashmap_remove(self, entry);
    hashmap_put_absent(self, entry);
    return old;
}

void *hashmap_remove(HashMap *self, void *key)
{
    HashMapEntry **e = find_entry(self, (HashMapEntry *)key);
    if (!*e) return NULL;

    HashMapEntry *old;
    old = *e;
    *e = old->next;
    old->next = NULL;

    self->count--;
    if (self->count < self->shrink_at) rehash(self, self->size >> 1);

    return old;
}

void hashmap_visit(HashMap *self, hash_visit_t visit, void *arg)
{
    if (!self || !self->entries || !visit) return;

    HashMapEntry *e;
    HashMapEntry *next;
    int entries;

rehashed:
    entries = self->size;
    for (int i = 0; i < entries; i++) {
        e = self->entries[i];
        while (e) {
            next = e->next;
            visit(e, arg);
            if (entries != self->size) {
                printf("hashmap is rehashed\n");
                goto rehashed;
            }
            e = next;
        }
    }
}

#ifdef __cplusplus
}
#endif
