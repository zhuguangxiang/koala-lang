/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc/gc.h"
#include "object.h"
#include "util/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

/* final class Map<K, V> {} */

typedef struct _MapEntry MapEntry;
typedef struct _KlMap KlMap;

#define MAP_DEFAULT_SIZE 16
#define MAP_LOAD_FACTOR  65

struct _MapEntry {
    MapEntry *next;
    int32 hash;
    anyref key;
    anyref val;
};

struct _KlMap {
    /* virt table */
    VTable *vtbl;
    /* collision list array */
    MapEntry **entries;
    /* type param bitmap */
    uint32 tp_map;
    /* entries array size */
    uint32 size;
    /* total number of entries */
    uint32 count;
    /* expand entries point */
    uint32 grow_at;
    /* shrink entries point */
    uint32 shrink_at;
};

static int entry_none_objmap[] = {
    1,
    offsetof(MapEntry, next),
};

static int entry_key_objmap[] = {
    2,
    offsetof(MapEntry, next),
    offsetof(MapEntry, key),
};

static int entry_val_objmap[] = {
    2,
    offsetof(MapEntry, next),
    offsetof(MapEntry, val),
};

static int entry_both_objmap[] = {
    3,
    offsetof(MapEntry, next),
    offsetof(MapEntry, key),
    offsetof(MapEntry, val),
};

static TypeInfo map_type = {
    .name = "Map",
    .flags = TF_CLASS | TF_FINAL,
};

void init_map_type(void)
{
    MethodDef map_methods[] = {
        /* clang-format off */
        /* clang-format on */
    };

    type_add_methdefs(&map_type, map_methods);
    type_ready(&map_type);
    type_show(&map_type);
    pkg_add_type(root_pkg, &map_type);
}

static int map_objmap[] = {
    1,
    offsetof(MapObj, entries),
};

static void alloc_entries(MapObj *map, uint32 size)
{
    GC_STACK(1);
    gc_push(&map, 0);

    MapEntry **entries = gc_alloc_array(size, sizeof(MapEntry *), 1);

    map->size = size;
    map->entries = entries;

    /* set thresholds */
    map->grow_at = size * MAP_LOAD_FACTOR / 100;
    if (size <= MAP_DEFAULT_SIZE)
        map->shrink_at = 0;
    else
        map->shrink_at = map->grow_at / 5;

    gc_pop();
}

objref map_new(uint32 tp_map)
{
    MapObj *map = gc_alloc(sizeof(*map), map_objmap);
    GC_STACK(1);
    gc_push(&map, 0);
    alloc_entries(map, MAP_DEFAULT_SIZE);
    map->vtbl = map_type.vtbl[0];
    map->tp_map = tp_map;
    gc_pop();
    return (objref)map;
}

static int32 __hash(anyref key, int ref)
{
    return tp_any_hash(key, ref);
}

static bool __equal(MapEntry *e, anyref key, int32 hash, int ref)
{
    return (e->hash == hash) && tp_any_equal(key, e->key, ref);
}

static inline int bucket(MapObj *map, int32 hash)
{
    return hash & (map->size - 1);
}

static MapEntry **find_entry(MapObj *map, anyref key)
{
    int ref = tp_is_ref(map->tp_map, 0);
    int32 hash = __hash(key, ref);

    MapEntry **e = &map->entries[bucket(map, hash)];
    while (*e && !__equal(*e, key, hash, ref)) e = &(*e)->next;
    return e;
}

static MapEntry *entry_new(anyref key, anyref val, int key_ref, int val_ref)
{
    int *objmap;

    GC_STACK(2);

    if (key_ref && val_ref) {
        gc_push(&key, 0);
        gc_push(&val, 1);
        objmap = entry_both_objmap;
    } else if (key_ref && !val_ref) {
        gc_push(&key, 0);
        objmap = entry_key_objmap;
    } else if (!key_ref && val_ref) {
        gc_push(&val, 0);
        objmap = entry_val_objmap;
    } else {
        objmap = entry_none_objmap;
    }

    MapEntry *entry = gc_alloc(sizeof(MapEntry), objmap);
    entry->hash = __hash(key, key_ref);
    entry->key = key;
    entry->val = val;

    gc_pop();

    return entry;
}

static void rehash(MapObj *map, int newsize)
{
    printf("map: rehashing: %d\n", map->count);

    int oldsize = map->size;
    MapEntry **oldentries = map->entries;

    GC_STACK(2);
    gc_push(&map, 0);
    gc_push(&oldentries, 1);

    alloc_entries(map, newsize);

    MapEntry *e, *n;
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

bool map_put_absent(objref self, anyref key, anyref val)
{
    MapObj *map = (MapObj *)self;

    if (*find_entry(map, key)) return 0;

    GC_STACK(1);
    gc_push(&map, 0);

    int key_ref = tp_is_ref(map->tp_map, 0);
    int val_ref = tp_is_ref(map->tp_map, 1);
    MapEntry *entry = entry_new(key, val, key_ref, val_ref);
    int b = bucket(map, entry->hash);
    entry->next = map->entries[b];
    map->entries[b] = entry;
    map->count++;
    if (map->count > map->grow_at) rehash(map, map->size << 1);

    gc_pop();
    return 1;
}

void map_put(objref self, anyref key, anyref val, anyref *old)
{
    MapObj *map = (MapObj *)self;
    MapEntry **entry = find_entry(map, key);
    if (*entry) {
        if (old) *old = (*entry)->val;
        (*entry)->val = val;
    } else {
        map_put_absent(self, key, val);
    }
}

bool map_get(objref self, anyref key, anyref *val)
{
    MapObj *map = (MapObj *)self;
    MapEntry **entry = find_entry(map, key);
    if (!*entry) return 0;
    *val = (*entry)->val;
    return 1;
}

bool map_remove(objref self, anyref key, anyref *val)
{
    MapObj *map = (MapObj *)self;
    MapEntry **entry = find_entry(map, key);
    if (!*entry) return 0;

    MapEntry *old = *entry;
    *entry = old->next;
    old->next = null;

    GC_STACK(1);
    gc_push(&old, 0);

    map->count--;
    if (map->count < map->shrink_at) rehash(map, map->size >> 1);

    if (val) *val = old->val;

    gc_pop();

    return 1;
}

#ifdef __cplusplus
}
#endif
