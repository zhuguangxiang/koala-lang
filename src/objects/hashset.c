/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "hashset.h"
#include "excobj.h"
#include "listobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hashset entry: only a key, no value. HashMapEntry MUST stay the first
 * member so entry pointers cast directly to HashMapEntry* / HashSetEntry*. */
typedef struct _HashSetEntry {
    HashMapEntry hnode;
    TValue key;
} HashSetEntry;

/* Iterator over hashset elements. Iteration order is unspecified. */
typedef struct _HashSetIterObject {
    OBJECT_HEAD;
    HashSetObject *set;
    int64_t step;
    HashMapIter iter;
} HashSetIterObject;

static int _hashset_entry_eq_(void *a, void *b)
{
    HashSetEntry *entry_a = (HashSetEntry *)a;
    HashSetEntry *entry_b = (HashSetEntry *)b;

    TValue *key_a = &entry_a->key;
    TValue *key_b = &entry_b->key;

    // fast-path

    if (is_int(key_a) && is_int(key_b)) {
        // int64/32/16/8 doesn't have cmp function
        return key_a->ival == key_b->ival;
    }

    if (is_float(key_a) && is_float(key_b)) {
        // float64/32/16/8 doesn't have cmp function
        return key_a->fval == key_b->fval;
    }

    if (is_uint(key_a) && is_uint(key_b)) {
        // uint64/32/16/8 doesn't have cmp function
        return (uint64_t)key_a->ival == (uint64_t)key_b->ival;
    }

    if (is_bool(key_a) && is_bool(key_b)) {
        // bool doesn't have cmp function
        return key_a->ival == key_b->ival;
    }

    TValue ret = kl_call_eq(key_a, key_b);
    // TODO: how to handle error in hashmap
    if (is_error(&ret)) return 0;

    return to_bool(&ret);
}

/* Forward declaration — _hashset_init reuses update logic for the
 * optional Iterable parameter. */
static TValue _hashset_update(TValue *self, TValue *args, int nargs);

static TValue _hashset_init(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    hashmap_init(&set->map, _hashset_entry_eq_);
    if (is_nil(&args[0])) return no_value;
    return _hashset_update(self, args, nargs);
}

static TValue _hashset_len(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 0);
    return int64_value(set->map.count);
}

static TValue _hashset_empty(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 0);
    return bool_value(set->map.count == 0);
}

static TValue _hashset_contains(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    HashSetEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    HashSetEntry *entry = hashmap_get(&set->map, &key_entry);
    return bool_value(entry != NULL);
}

/* Insert `key` if absent and return true; leave the set untouched and
 * return false if an equal key is already present. Unlike _dict_insert,
 * there is no value slot to overwrite, so the existing-entry branch is a
 * pure no-op. */
static TValue _hashset_add(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    HashSetEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    HashSetEntry *entry = hashmap_get(&set->map, &key_entry);
    if (entry) return bool_value(0);

    entry = mm_alloc(sizeof(HashSetEntry));
    entry->key = *key;
    hashmap_entry_init(entry, hash);
    hashmap_put(&set->map, entry);
    return bool_value(1);
}

/* Visitor passed to hashmap_fini() to release each HashSetEntry. The
 * entry pointer is the HashMapEntry* which is also the HashSetEntry*
 * because HashMapEntry is the first member. */
static void _hashset_entry_free_(void *entry, void *arg) { mm_free(entry); }

/* Remove `v` from the set. Return true if the element was present and
 * was removed, false if it was absent — no panic, unlike dict.remove
 * which must return V and therefore panics on a missing key. Aligns
 * with Java Set.remove / Rust HashSet::remove semantics (bool return,
 * silent on miss); Python's set.remove raises KeyError and set.discard
 * returns None, so neither maps onto this signature directly. */
static TValue _hashset_remove(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    HashSetEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    HashSetEntry *entry = hashmap_remove(&set->map, &key_entry);
    if (!entry) return bool_value(0);
    mm_free(entry);
    return bool_value(1);
}

static TValue _hashset_clear(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 0);
    /* Release every entry plus the bucket array, then reinitialize so
     * the set is usable again with an empty map. */
    hashmap_fini(&set->map, _hashset_entry_free_, NULL);
    hashmap_init(&set->map, _hashset_entry_eq_);
    return no_value;
}

/* Snapshot all elements into a fresh list. Iteration order is
 * unspecified (hashset contract); callers must not rely on it. Mirrors
 * the _dict_keys / _dict_values build-a-list-from-map pattern, but
 * returns the list directly instead of wrapping it in an iterator. */
static TValue _hashset_to_list(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 0);
    Object *list = kl_new_list();
    HashMapIter it;
    hashmap_iter_init(&set->map, &it, 0, 1);
    while (hashmap_iter_next(&it)) {
        HashSetEntry *entry = hashmap_iter_entry(&it);
        kl_list_append(list, entry->key);
    }
    return obj_value(list);
}

/* pub func update(items Iterable[T])
 *
 * Add all elements from `items` to the set in place. */
static TValue _hashset_update(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&src_set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (!hashmap_get(&set->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = e->key;
                hashmap_entry_init(ne, e->hnode.hash);
                hashmap_put(&set->map, ne);
            }
        }
        return no_value;
    }

    if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) return error_value;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&set->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&set->map, ne);
            }
        }
        return no_value;
    }

    if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) return error_value;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&set->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&set->map, ne);
            }
        }
        return no_value;
    }

    UNREACHABLE();
}

/* pub func __bitor__(other Iterable[T]) hashset[T]
 *
 * Return a new set with elements from self and other (self | other).
 * The receiver is left untouched. */
static TValue _hashset_bitor(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    /* Clone self into a fresh hashset. */
    Object *res = kl_new_hashset();
    HashSetObject *out = (HashSetObject *)res;
    HashMapIter clone_it;
    hashmap_iter_init(&set->map, &clone_it, 0, 1);
    while (hashmap_iter_next(&clone_it)) {
        HashSetEntry *e = hashmap_iter_entry(&clone_it);
        HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
        ne->key = e->key;
        hashmap_entry_init(ne, e->hnode.hash);
        hashmap_put(&out->map, ne);
    }

    /* Merge other into the clone. */
    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&src_set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (!hashmap_get(&out->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = e->key;
                hashmap_entry_init(ne, e->hnode.hash);
                hashmap_put(&out->map, ne);
            }
        }
    } else if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) return error_value;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&out->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&out->map, ne);
            }
        }
    } else if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) return error_value;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&out->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&out->map, ne);
            }
        }
    } else {
        UNREACHABLE();
    }

    return obj_value(res);
}

/* pub func __bitand__(other Iterable[T]) hashset[T]
 *
 * Return a new set with elements common to self and other (self & other).
 * Iterates `other` and keeps only elements present in self — O(m) with
 * O(1) lookups, no temporary materialization needed. */
static TValue _hashset_bitand(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    Object *res = kl_new_hashset();
    HashSetObject *out = (HashSetObject *)res;

    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&src_set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (hashmap_get(&set->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = e->key;
                hashmap_entry_init(ne, e->hnode.hash);
                hashmap_put(&out->map, ne);
            }
        }
    } else if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) return error_value;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (hashmap_get(&set->map, &tmp)) {
                HashSetEntry ne_tmp = { .key = item };
                hashmap_entry_init(&ne_tmp, h);
                if (!hashmap_get(&out->map, &ne_tmp)) {
                    HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                    ne->key = item;
                    hashmap_entry_init(ne, h);
                    hashmap_put(&out->map, ne);
                }
            }
        }
    } else if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) return error_value;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (hashmap_get(&set->map, &tmp)) {
                HashSetEntry ne_tmp = { .key = item };
                hashmap_entry_init(&ne_tmp, h);
                if (!hashmap_get(&out->map, &ne_tmp)) {
                    HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                    ne->key = item;
                    hashmap_entry_init(ne, h);
                    hashmap_put(&out->map, ne);
                }
            }
        }
    } else {
        UNREACHABLE();
    }

    return obj_value(res);
}

/* pub func __sub__(other Iterable[T]) hashset[T]
 *
 * Return a new set with elements in self that are not in other
 * (self - other). Clones self then removes every element of other. */
static TValue _hashset_sub(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    /* Clone self. */
    Object *res = kl_new_hashset();
    HashSetObject *out = (HashSetObject *)res;
    HashMapIter clone_it;
    hashmap_iter_init(&set->map, &clone_it, 0, 1);
    while (hashmap_iter_next(&clone_it)) {
        HashSetEntry *e = hashmap_iter_entry(&clone_it);
        HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
        ne->key = e->key;
        hashmap_entry_init(ne, e->hnode.hash);
        hashmap_put(&out->map, ne);
    }

    /* Remove every element of other from the clone. */
    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&src_set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            HashSetEntry *found = hashmap_remove(&out->map, &tmp);
            if (found) mm_free(found);
        }
    } else if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) return error_value;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            HashSetEntry *found = hashmap_remove(&out->map, &tmp);
            if (found) mm_free(found);
        }
    } else if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) return error_value;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            HashSetEntry *found = hashmap_remove(&out->map, &tmp);
            if (found) mm_free(found);
        }
    } else {
        UNREACHABLE();
    }

    return obj_value(res);
}

/* pub func is_subset(other Iterable[T]) bool
 *
 * Return true if every element in self is in other.
 * Fast-path: other is hashset → O(n) with O(1) lookups, no allocation.
 * General: materialize other into a temp hashset, then check. */
static TValue _hashset_is_subset(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    /* Fast-path: other is a hashset. */
    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (!hashmap_get(&src_set->map, &tmp)) return bool_value(0);
        }
        return bool_value(1);
    }

    /* General path: materialize other into temp hashset. */
    HashSetObject *temp = (HashSetObject *)kl_new_hashset();

    if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) goto cleanup_err;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) goto cleanup_err;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&temp->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&temp->map, ne);
            }
        }
    } else if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) goto cleanup_err;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) goto cleanup_err;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&temp->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&temp->map, ne);
            }
        }
    } else {
        UNREACHABLE();
    }

    /* Check self ⊆ temp. */
    {
        int result = 1;
        HashMapIter it;
        hashmap_iter_init(&set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (!hashmap_get(&temp->map, &tmp)) { result = 0; break; }
        }
        hashmap_fini(&temp->map, _hashset_entry_free_, NULL);
        mm_free(temp);
        return bool_value(result);
    }

cleanup_err:
    hashmap_fini(&temp->map, _hashset_entry_free_, NULL);
    mm_free(temp);
    return error_value;
}

/* pub func is_superset(other Iterable[T]) bool
 *
 * Return true if every element in other is in self.
 * Iterates other and checks membership in self (always O(1) hashmap_get).
 * Short-circuits on the first miss. */
static TValue _hashset_is_superset(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&src_set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (!hashmap_get(&set->map, &tmp)) return bool_value(0);
        }
        return bool_value(1);
    }

    if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) return error_value;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&set->map, &tmp)) return bool_value(0);
        }
        return bool_value(1);
    }

    if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) return error_value;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (!hashmap_get(&set->map, &tmp)) return bool_value(0);
        }
        return bool_value(1);
    }

    UNREACHABLE();
}

/* pub func sym_diff(other Iterable[T]) hashset[T]
 *
 * Return a new set with elements in self or other, but not both.
 * Algorithm: clone self, then for each elem in other:
 *   - if elem in self → remove from result (in both → exclude)
 *   - if elem not in self → add to result (only in other → include)
 * Duplicates in other are harmless: remove is idempotent on a missing
 * key, add checks before inserting. O(n+m). */
static TValue _hashset_sym_diff(TValue *self, TValue *args, int nargs)
{
    HashSetObject *set = SELF_AS(hashset_type);
    ASSERT(nargs == 1);
    TValue *src = &args[0];
    Object *src_obj = to_obj(src);

    /* Clone self into result. */
    Object *res = kl_new_hashset();
    HashSetObject *out = (HashSetObject *)res;
    HashMapIter clone_it;
    hashmap_iter_init(&set->map, &clone_it, 0, 1);
    while (hashmap_iter_next(&clone_it)) {
        HashSetEntry *e = hashmap_iter_entry(&clone_it);
        HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
        ne->key = e->key;
        hashmap_entry_init(ne, e->hnode.hash);
        hashmap_put(&out->map, ne);
    }

    if (IS_HASHSET(src_obj)) {
        HashSetObject *src_set = (HashSetObject *)src_obj;
        HashMapIter it;
        hashmap_iter_init(&src_set->map, &it, 0, 1);
        while (hashmap_iter_next(&it)) {
            HashSetEntry *e = hashmap_iter_entry(&it);
            HashSetEntry tmp = { .key = e->key };
            hashmap_entry_init(&tmp, e->hnode.hash);
            if (hashmap_get(&set->map, &tmp)) {
                HashSetEntry *found = hashmap_remove(&out->map, &tmp);
                if (found) mm_free(found);
            } else if (!hashmap_get(&out->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = e->key;
                hashmap_entry_init(ne, e->hnode.hash);
                hashmap_put(&out->map, ne);
            }
        }
    } else if (kl_is_seq(src_obj)) {
        TValue len_tv = kl_call_len(src);
        if (is_error(&len_tv)) return error_value;
        size_t n = (size_t)to_int64(&len_tv);
        for (size_t i = 0; i < n; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (hashmap_get(&set->map, &tmp)) {
                HashSetEntry *found = hashmap_remove(&out->map, &tmp);
                if (found) mm_free(found);
            } else if (!hashmap_get(&out->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&out->map, ne);
            }
        }
    } else if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
        if (is_error(&it)) return error_value;
        while (1) {
            TValue item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            HashSetEntry tmp = { .key = item };
            unsigned int h = kl_call_hash(&item);
            hashmap_entry_init(&tmp, h);
            if (hashmap_get(&set->map, &tmp)) {
                HashSetEntry *found = hashmap_remove(&out->map, &tmp);
                if (found) mm_free(found);
            } else if (!hashmap_get(&out->map, &tmp)) {
                HashSetEntry *ne = mm_alloc(sizeof(HashSetEntry));
                ne->key = item;
                hashmap_entry_init(ne, h);
                hashmap_put(&out->map, ne);
            }
        }
    } else {
        UNREACHABLE();
    }

    return obj_value(res);
}

static MethodDef hashset_methods[] = {
    { "__init__", _hashset_init },
    { "len", _hashset_len },
    { "empty", _hashset_empty },
    { "__contains__", _hashset_contains },
    { "add", _hashset_add },
    { "remove", _hashset_remove },
    { "clear", _hashset_clear },
    { "to_list", _hashset_to_list },
    { "update", _hashset_update },
    { "is_subset", _hashset_is_subset },
    { "is_superset", _hashset_is_superset },
    { "__bitor__", _hashset_bitor },
    { "__bitand__", _hashset_bitand },
    { "__sub__", _hashset_sub },
    { "union", _hashset_bitor },
    { "intersect", _hashset_bitand },
    { "diff", _hashset_sub },
    { "sym_diff", _hashset_sym_diff },
    { NULL },
};

/* pub class hashset[T] : MutableSet[T] { ... } */
TypeObject hashset_type = {
    ._type = &type_type,
    .name = "hashset",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(HashSetObject),
    .methdefs = hashset_methods,
};

/* __init__(s hashset[T], step int): panics on step <= 0. Mirrors
 * _init_dict_iter with reverse hard-wired to 0 — hashset has no
 * reversed() counterpart. */
static TValue _hashset_iter_init(TValue *self, TValue *args, int nargs)
{
    HashSetIterObject *iter = SELF_AS(hashset_iter_type);
    ASSERT(nargs == 2);
    HashSetObject *set = kl_arg_obj_as(0, hashset_type);
    int64_t step = kl_arg_int64(1);
    if (step <= 0) {
        raise_exc("step must be positive");
        return error_value;
    }
    iter->set = set;
    iter->step = step;
    hashmap_iter_init(&set->map, &iter->iter, 0, step);
    return no_value;
}

static TValue _hashset_iter_has_next(TValue *self, TValue *args, int nargs)
{
    HashSetIterObject *iter = SELF_AS(hashset_iter_type);
    ASSERT(nargs == 0);
    return bool_value(hashmap_iter_has_next(&iter->iter));
}

/* next() T? — returns nil at exhaustion, matching dict_keys_iter_next. */
static TValue _hashset_iter_next(TValue *self, TValue *args, int nargs)
{
    HashSetIterObject *iter = SELF_AS(hashset_iter_type);
    ASSERT(nargs == 0);
    if (!hashmap_iter_next(&iter->iter)) return nil_value;
    HashSetEntry *entry = hashmap_iter_entry(&iter->iter);
    return entry->key;
}

/* next_strict() T — panics at exhaustion. */
static TValue _hashset_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    TValue ret = _hashset_iter_next(self, args, nargs);
    if (is_nil(&ret)) {
        raise_exc("No more elements in HashSetIter");
        return error_value;
    }
    return ret;
}

static MethodDef hashset_iter_methods[] = {
    { "__init__", _hashset_iter_init },
    { "has_next", _hashset_iter_has_next },
    { "next", _hashset_iter_next },
    { "next_strict", _hashset_iter_next_strict },
    { NULL },
};

/* class HashSetIter[T] : Iterator[T] { ... } */
TypeObject hashset_iter_type = {
    ._type = &type_type,
    .name = "HashSetIter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(HashSetIterObject),
    .methdefs = hashset_iter_methods,
};

Object *kl_new_hashset(void)
{
    HashSetObject *set = mm_alloc_obj(set);
    INIT_OBJECT_HEAD(set, &hashset_type, 0);
    hashmap_init(&set->map, _hashset_entry_eq_);
    return (Object *)set;
}

#ifdef __cplusplus
}
#endif
