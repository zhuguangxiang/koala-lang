/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
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

static void __alloc_entries(hashmap_t *self, int size)
{
    self->size = size;
    self->entries = malloc(size * sizeof(hashmap_entry_t *));
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

void hashmap_init(hashmap_t *self, hashmap_equal_t equal)
{
    memset(self, 0, sizeof(hashmap_t));
    self->equal = equal;
    __alloc_entries(self, HASHMAP_INITIAL_SIZE);
}

void hashmap_fini(hashmap_t *self, hashmap_visit_t _free, void *arg)
{
    if (!self || !self->entries) return;

    hashmap_entry_t *e, *nxt;
    for (int i = 0; i < self->size; i++) {
        e = self->entries[i];
        while (e) {
            nxt = e->next;
            _free(e, arg);
            e = nxt;
        }
    }

    free(self->entries);
}

static inline int bucket(hashmap_t *self, hashmap_entry_t *e)
{
    return e->hash & (self->size - 1);
}

static inline int entry_equals(
    hashmap_t *self, hashmap_entry_t *e1, hashmap_entry_t *e2)
{
    return (e1 == e2) || (e1->hash == e2->hash && self->equal(e1, e2));
}

static inline hashmap_entry_t **find_entry(
    hashmap_t *self, hashmap_entry_t *key)
{
    hashmap_entry_t **e = &self->entries[bucket(self, key)];
    while (*e && !entry_equals(self, *e, key)) e = &(*e)->next;
    return e;
}

void *hashmap_get(hashmap_t *self, void *key)
{
    if (!self) return NULL;
    return *find_entry(self, key);
}

static void rehash(hashmap_t *self, int newsize)
{
    int oldsize = self->size;
    printf("hashmap:\nrehashing: %d\n", self->count);
    hashmap_entry_t **oldentries = self->entries;

    __alloc_entries(self, newsize);

    hashmap_entry_t *e;
    hashmap_entry_t *n;
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

    free(oldentries);
}

int hashmap_put_absent(hashmap_t *self, void *entry)
{
    if (self == NULL) return -1;

    hashmap_entry_t *e = entry;

    if (*find_entry(self, e)) return -1;

    int b = bucket(self, e);
    e->next = self->entries[b];
    self->entries[b] = e;
    self->count++;
    if (self->count > self->grow_at) rehash(self, self->size << 1);
    return 0;
}

void *hashmap_put(hashmap_t *self, void *entry)
{
    void *old = hashmap_remove(self, entry);
    hashmap_put_absent(self, entry);
    return old;
}

void *hashmap_remove(hashmap_t *self, void *key)
{
    hashmap_entry_t **e = find_entry(self, (hashmap_entry_t *)key);
    if (!*e) return NULL;

    hashmap_entry_t *old;
    old = *e;
    *e = old->next;
    old->next = NULL;

    self->count--;
    if (self->count < self->shrink_at) rehash(self, self->size >> 1);

    return old;
}

void hashmap_visit(hashmap_t *self, hashmap_visit_t visit, void *arg)
{
    if (!self || !self->entries || !visit) return;

    hashmap_entry_t *e;
    hashmap_entry_t *next;
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
