/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "atom.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Symbol table for module and type                                         |
 +---------------------------------------------------------------------------*/

/* object symbol entry */
typedef struct _SymbolEntry {
    HashMapEntry hnode;
    char *key;
    int len;
    Object *obj;
} SymbolEntry;

static int __stbl_equal__(void *e1, void *e2)
{
    SymbolEntry *n1 = e1;
    SymbolEntry *n2 = e2;

    if (n1->key == n2->key) return 1;

    if (n1->len != n2->len) return 0;
    if (!strncmp(n1->key, n2->key, n1->len)) return 1;
    return 0;
}

void stbl_add_obj(HashMap *map, char *name, Object *obj)
{
    SymbolEntry *e = mm_alloc_obj(e);
    uint64_t hash = str_hash(name);
    hashmap_entry_init(e, hash);
    int len = strlen(name);
    e->key = atom_nstr(name, len);
    e->len = len;
    e->obj = obj;
    int r = hashmap_put_absent(map, e);
    ASSERT(!r);
}

Object *stbl_find_obj(HashMap *map, char *name)
{
    int len = strlen(name);
    SymbolEntry entry = { .key = name, .len = len };
    uint64_t hash = mem_hash(name, len);
    hashmap_entry_init(&entry, hash);
    SymbolEntry *found = hashmap_get(map, &entry);
    return found ? found->obj : NULL;
}

void stbl_init(HashMap *map) { hashmap_init(map, __stbl_equal__); }

#ifdef __cplusplus
}
#endif
