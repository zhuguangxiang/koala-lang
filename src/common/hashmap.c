/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "hashmap.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FNV32_BASE  ((unsigned int)0x811c9dc5)
#define FNV32_PRIME ((unsigned int)0x01000193)

unsigned int str_hash(const char *str)
{
    unsigned int c, hash = FNV32_BASE;
    while ((c = (unsigned char)*str++)) hash = (hash * FNV32_PRIME) ^ c;
    return hash;
}

unsigned int mem_hash(const void *buf, int len)
{
    unsigned int hash = FNV32_BASE;
    unsigned char *ucbuf = (unsigned char *)buf;
    while (len--) {
        unsigned int c = *ucbuf++;
        hash = (hash * FNV32_PRIME) ^ c;
    }
    return hash;
}

#define HASHMAP_INITIAL_SIZE 16
#define HASHMAP_LOAD_FACTOR  65

static void __alloc_entries(HashMap *self, int size)
{
    self->size = size;
    self->entries = mm_alloc(size * sizeof(HListHead));

    for (int i = 0; i < size; i++) {
        init_hlist_head(self->entries + i);
    }

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
    init_list(&self->ord_list);
    __alloc_entries(self, HASHMAP_INITIAL_SIZE);
}

void hashmap_fini(HashMap *self, HashMapVisitFunc _free, void *arg)
{
    if (!self || !self->entries) return;

    HashMapEntry *e, *nxt;
    list_foreach_safe(e, nxt, ord_node, &self->ord_list) {
        list_remove(&e->ord_node);
        hlist_del(&e->hnode);
        if (_free) _free(e, arg);
    }

    mm_free(self->entries);
}

static inline int bucket(HashMap *self, HashMapEntry *e)
{
    return e->hash & (self->size - 1);
}

static inline int entry_equals(HashMap *self, HashMapEntry *e1, HashMapEntry *e2)
{
    return (e1 == e2) || (e1->hash == e2->hash && self->equal(e1, e2));
}

static inline void *find_entry(HashMap *self, HashMapEntry *key)
{
    int b = bucket(self, key);

    HListNode *node;
    hlist_for_each(node, self->entries + b) {
        if (self->equal(node, key)) return node;
    }

    return NULL;
}

void *hashmap_get(HashMap *self, void *key)
{
    if (!self) return NULL;
    return find_entry(self, key);
}

static void rehash(HashMap *self, int newsize)
{
    int oldsize = self->size;
    HListHead *oldentries = self->entries;

    __alloc_entries(self, newsize);

    HListNode *node, *nxt;
    for (int i = 0; i < oldsize; i++) {
        hlist_for_each_safe(node, nxt, oldentries + i) {
            hlist_del(node);
            int b = bucket(self, (HashMapEntry *)node);
            hlist_add(node, self->entries + b);
        }
    }

    mm_free(oldentries);
}

void hashmap_put_only(HashMap *self, void *entry)
{
    HashMapEntry *e = entry;
    if (!hlist_unhashed(&e->hnode)) return;

    int b = bucket(self, e);
    hlist_add(&e->hnode, self->entries + b);
    list_push_back(&self->ord_list, &e->ord_node);

    self->count++;
    if (self->count > self->grow_at) rehash(self, self->size << 1);
}

int hashmap_put_absent(HashMap *self, void *entry)
{
    if (!self) return -1;
    HashMapEntry *e = entry;
    if (find_entry(self, e)) return -1;
    hashmap_put_only(self, entry);
    return 0;
}

void *hashmap_put(HashMap *self, void *entry)
{
    void *old = hashmap_remove(self, entry);
    hashmap_put_only(self, entry);
    return old;
}

void *hashmap_remove(HashMap *self, void *key)
{
    HashMapEntry *e = find_entry(self, key);
    if (!e) return NULL;

    hlist_del(&e->hnode);
    list_remove(&e->ord_node);

    self->count--;
    if (self->count < self->shrink_at) rehash(self, self->size >> 1);

    return e;
}

int hashmap_next(HashMap *self, HashMapIter *it)
{
    if (!self || !self->entries) return 0;

    HashMapEntry *next;
    if (!it->state) {
        next = list_first(&self->ord_list, HashMapEntry, ord_node);
        if (!next) return 0;

        it->entry = next;
        it->next = list_next(next, ord_node, &self->ord_list);
        it->state = 1;
        return 1;
    }

    next = it->next;
    if (!next) {
        it->entry = NULL;
        it->state = 0;
        return 0;
    }

    it->entry = next;
    it->next = list_next(next, ord_node, &self->ord_list);
    return 1;
}

int hashmap_prev(HashMap *self, HashMapIter *it)
{
    if (!self || !self->entries) return 0;

    HashMapEntry *next;
    if (!it->state) {
        next = list_last(&self->ord_list, HashMapEntry, ord_node);
        if (!next) return 0;

        it->entry = next;
        it->next = list_prev(next, ord_node, &self->ord_list);
        it->state = 1;
        return 1;
    }

    next = it->next;
    if (!next) {
        it->entry = NULL;
        it->state = 0;
        return 0;
    }

    it->entry = next;
    it->next = list_prev(next, ord_node, &self->ord_list);
    return 1;
}

#ifdef __cplusplus
}
#endif
