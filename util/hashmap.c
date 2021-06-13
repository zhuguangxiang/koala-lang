/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "hashmap.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

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
}

void hashmap_init(HashMap *self, HashMapEqualFunc equal)
{
    memset(self, 0, sizeof(*self));
    self->equal = equal;
    __alloc_entries(self, HASHMAP_INITIAL_SIZE);
}

void hashmap_fini(HashMap *self, HashMapVisitFunc _free, void *arg)
{
    if (!self || !self->entries) return;

    HashMapEntry *e, *nxt;
    for (int i = 0; i < self->size; i++) {
        e = self->entries[i];
        while (e) {
            nxt = e->next;
            _free(e, arg);
            e = nxt;
        }
    }

    mm_free(self->entries);
}

static inline int bucket(HashMap *self, HashMapEntry *e)
{
    return e->hash & (self->size - 1);
}

static inline int entry_equals(HashMap *self, HashMapEntry *e1,
                               HashMapEntry *e2)
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
    if (!self) return nil;
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
    if (!self) return -1;

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
    HashMapEntry **e = find_entry(self, key);
    if (!*e) return nil;

    HashMapEntry *old;
    old = *e;
    *e = old->next;
    old->next = nil;

    self->count--;
    if (self->count < self->shrink_at) rehash(self, self->size >> 1);

    return old;
}

void hashmap_visit(HashMap *self, HashMapVisitFunc visit, void *arg)
{
    if (!self || !self->entries || !visit) return;

    HashMapEntry *e, *next;
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
