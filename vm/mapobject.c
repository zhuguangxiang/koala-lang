/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "arrayobject.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FNV32_BASE  ((unsigned int)0x811c9dc5)
#define FNV32_PRIME ((unsigned int)0x01000193)

uint32 str_hash(const char *str)
{
    uint32 c, hash = FNV32_BASE;
    while ((c = (unsigned char)*str++)) hash = (hash * FNV32_PRIME) ^ c;
    return hash;
}

uint32 mem_hash(const void *buf, int len)
{
    uint32 hash = FNV32_BASE;
    unsigned char *ucbuf = (unsigned char *)buf;
    while (len--) {
        uint32 c = *ucbuf++;
        hash = (hash * FNV32_PRIME) ^ c;
    }
    return hash;
}

#define MAP_INIT_SIZE   16
#define MAP_LOAD_FACTOR 65

typedef struct _MapEntry {
    struct _MapEntry *next;
    uint32 hash;
    uintptr_t key;
    uintptr_t val;
} MapEntry, *MapEntryRef;

typedef struct _MapObject {
    OBJECT_HEAD
    /* collision list array */
    MapEntryRef *entries;
    /* entries array size */
    uint32 size;
    /* total number of entries */
    uint32 count;
    /* expand entries point */
    uint32 grow_at;
    /* shrink entries point */
    uint32 shrink_at;
    /* key is reference? */
    int8 key_ref;
    /* value is reference? */
    int8 val_ref;
} MapObject, *MapObjectRef;

static int __entry_none_objmap__[] = {
    1,
    offsetof(MapEntry, next),
};

static int __entry_key_objmap__[] = {
    2,
    offsetof(MapEntry, next),
    offsetof(MapEntry, key),
};

static int __entry_val_objmap__[] = {
    2,
    offsetof(MapEntry, next),
    offsetof(MapEntry, val),
};

static int __entry_both_objmap__[] = {
    3,
    offsetof(MapEntry, next),
    offsetof(MapEntry, key),
    offsetof(MapEntry, val),
};

static int __map_objmap__[] = {
    1,
    offsetof(MapObject, entries),
};

static void __alloc_entries(MapObjectRef map, uint32 size)
{
    GC_STACK(1);
    gc_push1(&map);

    MapEntryRef *entries = gc_alloc_array(size, sizeof(MapEntryRef), 1);

    map->size = size;
    map->entries = entries;

    /* set thresholds */
    map->grow_at = size * MAP_LOAD_FACTOR / 100;
    if (size <= MAP_INIT_SIZE)
        map->shrink_at = 0;
    else
        map->shrink_at = map->grow_at / 5;

    gc_pop();
}

typedef struct _EntryInfo {
    uint32 hash;
    uint8 key_ref;
    uint8 val_ref;
} EntryInfo;

static inline int bucket(MapObjectRef map, uint32 hash)
{
    return hash & (map->size - 1);
}

ObjectRef map_new(int8 key_ref, int8 val_ref)
{
    MapObjectRef map = gc_alloc(sizeof(MapObject), __map_objmap__);
    GC_STACK(1);
    gc_push1(&map);

    __alloc_entries(map, MAP_INIT_SIZE);

    /* set reference or not */
    map->key_ref = key_ref;
    map->val_ref = val_ref;

    gc_pop();
    return (ObjectRef)map;
}

static uint32 __hash(MapObjectRef map, uintptr_t key)
{
    if (!map->key_ref) return mem_hash(&key, sizeof(uintptr_t));

    TypeObjectRef type = ((ObjectRef)key)->type;
    // MethodObjectRef meth = type->vtbl[0][0];
    uint32 ret = 0;
    // method_call(meth, key, &ret);
    return ret;
}

static int __equal(MapObjectRef map, MapEntryRef e, uintptr_t key, uint32 hash)
{
    if (e->key == key) return 1;
    if (e->hash != hash) return 0;
    if (!map->key_ref) return 1;

    ObjectRef obj = (ObjectRef)key;
    TypeObjectRef type = ((ObjectRef)key)->type;
    // MethodObjectRef meth = type->vtbl[0][1];
    int ret = 0;
    // method_call(meth, key, &ret);
    return ret;
}

static MapEntryRef *find_entry(MapObjectRef map, uintptr_t key)
{
    uint32 hash = __hash(map, key);
    MapEntryRef *e = &map->entries[bucket(map, hash)];
    while (*e && !__equal(map, *e, key, hash)) e = &(*e)->next;
    return e;
}

MapEntryRef __entry_new(EntryInfo *info, uintptr_t key, uintptr_t val)
{
    int *objmap;

    GC_STACK(2);

    if (info->key_ref && info->val_ref) {
        gc_push2(&key, &val);
        objmap = __entry_both_objmap__;
    } else if (info->key_ref && !info->val_ref) {
        gc_push1(&key);
        objmap = __entry_key_objmap__;
    } else if (!info->key_ref && info->val_ref) {
        gc_push1(&val);
        objmap = __entry_val_objmap__;
    } else {
        objmap = __entry_none_objmap__;
    }

    MapEntryRef entry = gc_alloc(sizeof(MapEntry), objmap);
    entry->hash = info->hash;
    entry->key = key;
    entry->val = val;

    gc_pop();

    return entry;
}

static void rehash(MapObjectRef map, int newsize)
{
    printf("map: rehashing: %d\n", map->count);

    int oldsize = map->size;
    MapEntryRef *oldentries = map->entries;

    GC_STACK(2);
    gc_push2(&map, &oldentries);

    __alloc_entries(map, newsize);

    MapEntryRef e, n;
    int b;
    for (int i = 0; i < oldsize; ++i) {
        e = oldentries[i];
        while (e) {
            n = e->next;
            b = bucket(map, e->hash);
            e->next = map->entries[b];
            map->entries[b] = e;
            e = n;
        }
    }

    gc_pop();
}

int32 map_put_absent(ObjectRef self, uintptr_t key, uintptr_t val)
{
    MapObjectRef map = (MapObjectRef)self;

    if (*find_entry(map, key)) return -1;

    GC_STACK(1);
    gc_push1(&map);

    EntryInfo info = { __hash(map, key), map->key_ref, map->val_ref };
    MapEntryRef entry = __entry_new(&info, key, val);
    int b = bucket(map, entry->hash);
    entry->next = map->entries[b];
    map->entries[b] = entry;
    map->count++;
    if (map->count > map->grow_at) rehash(map, map->size << 1);

    gc_pop();
    return 0;
}

void map_put(ObjectRef self, uintptr_t key, uintptr_t val, uintptr_t *old_val)
{
    MapObjectRef map = (MapObjectRef)self;
    MapEntryRef *entry = find_entry(map, key);
    if (*entry) {
        if (old_val) *old_val = (*entry)->val;
        (*entry)->val = val;
    } else {
        map_put_absent(self, key, val);
    }
}

int32 map_get(ObjectRef self, uintptr_t key, uintptr_t *val)
{
    MapObjectRef map = (MapObjectRef)self;
    MapEntryRef *entry = find_entry(map, key);
    if (*entry) {
        *val = (*entry)->val;
        return 0;
    }
    return -1;
}

int32 map_remove(ObjectRef self, uintptr_t key, uintptr_t *val)
{
    MapObjectRef map = (MapObjectRef)self;
    MapEntryRef *entry = find_entry(map, key);
    if (!*entry) return -1;

    MapEntryRef old = *entry;
    *entry = old->next;
    old->next = NULL;

    map->count--;
    if (map->count < map->shrink_at) rehash(map, map->size >> 1);

    if (val) *val = old->val;
    return 0;
}

#ifdef __cplusplus
}
#endif
