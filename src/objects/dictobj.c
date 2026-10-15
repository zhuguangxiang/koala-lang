/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dictobj.h"
#include "excobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dictionary entry structure used internally by the dictionary object.

typedef struct _DictEntry {
    HashMapEntry hnode;
    TValue key;
    TValue value;
} DictEntry;

static int _dict_entry_eq_(void *a, void *b)
{
    DictEntry *entry_a = (DictEntry *)a;
    DictEntry *entry_b = (DictEntry *)b;

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

/* Insert a (key, value) mapping. If `key` already exists its value is
 * replaced in place and the original insertion slot is preserved; otherwise
 * a fresh DictEntry is allocated and appended to the ordered list. Shared
 * by __setitem__, update() and setdefault(). */
static void _dict_insert(DictObject *dict, TValue *key, TValue *value)
{
    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    if (entry) {
        entry->value = *value;
    } else {
        entry = mm_alloc(sizeof(DictEntry));
        entry->key = *key;
        entry->value = *value;
        hashmap_entry_init(entry, hash);
        hashmap_put(&dict->map, entry);
    }
}

static TValue _dict_init(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 0);
    hashmap_init(&dict->map, _dict_entry_eq_);
    return no_value;
}

static TValue _dict_len(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 0);
    return int64_value(dict->map.count);
}

static TValue _dict_empty(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 0);
    return bool_value(dict->map.count == 0);
}

static TValue _dict_fmt(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    Formatter *f = kl_arg_obj_as(0, fmt_type);

    Buffer *buf = &f->buf;

    buf_write_char(buf, '{');

    /* Iterate in insertion order via the ordered list. HashMapEntry is the
     * first member of DictEntry, so the entry pointer casts directly. */
    HashMapIter it = { 0 };
    int first = 1;
    while (hashmap_next(&dict->map, &it)) {
        DictEntry *entry = (DictEntry *)hashmap_entry(&it);
        if (!first) buf_write_str(buf, ", ");
        first = 0;
        kl_call_fmt(&entry->key, f);
        buf_write_str(buf, ": ");
        kl_call_fmt(&entry->value, f);
    }

    buf_write_char(buf, '}');
    return no_value;
}

static TValue _dict_set_item(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 2);
    _dict_insert(dict, &args[0], &args[1]);
    return no_value;
}

static TValue _dict_get_item(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    if (entry) {
        return entry->value;
    } else {
        raise_exc("Key not found");
        return error_value;
    }
}

static TValue _dict_contains(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    return bool_value(entry != NULL);
}

/* Visitor passed to hashmap_fini() to release each DictEntry. The entry
 * pointer is the HashMapEntry* which is also the DictEntry* because
 * HashMapEntry is the first member. */
static void _dict_entry_free_(void *entry, void *arg) { mm_free(entry); }

static TValue _dict_clear(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 0);
    /* Release every entry plus the bucket array, then reinitialize so the
     * dict is usable again with an empty ordered list. */
    hashmap_fini(&dict->map, _dict_entry_free_, NULL);
    hashmap_init(&dict->map, _dict_entry_eq_);
    return no_value;
}

static TValue _dict_remove(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_remove(&dict->map, &key_entry);
    if (!entry) {
        raise_exc("Key not found");
        return error_value;
    }
    TValue value = entry->value;
    mm_free(entry);
    return value;
}

static TValue _dict_remove_or(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 2);
    TValue *key = &args[0];
    TValue *default_value = &args[1];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_remove(&dict->map, &key_entry);
    if (!entry) return *default_value;
    TValue value = entry->value;
    mm_free(entry);
    return value;
}

static TValue _dict_get(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    TValue *key = &args[0];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    /* Missing key yields nil so callers can unwrap with `if let`. */
    if (!entry) return nil_value;
    return entry->value;
}

static TValue _dict_get_or(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 2);
    TValue *key = &args[0];
    TValue *default_value = &args[1];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    if (!entry) return *default_value;
    return entry->value;
}

/* Consume one element yielded by `src` in dict.update() and merge it into
 * `dict`. The signature `src Iterable[(K, V)]` is enforced by the compiler,
 * so every yielded element is statically a 2-tuple of (K, V); only ASSERT
 * here as a defensive invariant check, no runtime exception is warranted. */
static inline void _dict_update_one(DictObject *dict, TValue *item)
{
    Object *pair = to_obj(item);
    ASSERT(IS_TUPLE(pair) && TUPLE_SIZE(pair) == 2);
    TValue *kv = TUPLE_ITEMS(pair);
    _dict_insert(dict, &kv[0], &kv[1]);
}

static TValue _dict_update(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    Object *src = kl_arg_obj(0);

    /* Sequence fast-path (list / tuple / str / bytes): use len + __getitem__
     * because Sequence types do not register SLOT_ITER. */
    if (kl_is_seq(src)) {
        TValue len = kl_call_len(args);
        if (is_error(&len)) return error_value;
        size_t m = (size_t)to_int64(&len);
        for (size_t i = 0; i < m; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(args, &idx);
            if (is_error(&item)) return error_value;
            _dict_update_one(dict, &item);
        }
        return no_value;
    }

    /* Generic Iterable path: iter() + next() until next yields nil. */
    if (kl_is_iter(src)) {
        TValue it = kl_call_iter(args, 1);
        if (is_error(&it)) return error_value;

        TValue item;
        while (1) {
            item = kl_call_next(&it);
            if (is_error(&item)) return error_value;
            if (is_nil(&item)) break;
            _dict_update_one(dict, &item);
        }
        return no_value;
    }

    /* The compiler rejects non-Iterable arguments at the call site, so this
     * point is unreachable in a well-typed program. */
    UNREACHABLE();
}

static TValue _dict_setdefault(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 2);
    TValue *key = &args[0];
    TValue *default_value = &args[1];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    /* Key already present: return the existing value, do NOT overwrite. */
    if (entry) return entry->value;

    /* Key absent: insert (key, default_value) and return default_value. */
    entry = mm_alloc(sizeof(DictEntry));
    entry->key = *key;
    entry->value = *default_value;
    hashmap_entry_init(entry, hash);
    hashmap_put(&dict->map, entry);
    return *default_value;
}

static MethodDef dict_methods[] = {
    { "__init__", _dict_init },
    { "len", _dict_len },
    { "empty", _dict_empty },
    { "fmt", _dict_fmt },
    { "__setitem__", _dict_set_item },
    { "__getitem__", _dict_get_item },
    { "__contains__", _dict_contains },
    { "clear", _dict_clear },
    { "remove", _dict_remove },
    { "remove_or", _dict_remove_or },
    { "get", _dict_get },
    { "get_or", _dict_get_or },
    { "update", _dict_update },
    { "setdefault", _dict_setdefault },
    { NULL },
};

/* pub class dict[K, V] : Map[K, V] { ... } */
DEFINE_TYPE(dict, TP_FLAGS_CLASS, sizeof(DictObject), dict_methods, NULL);

Object *kl_new_dict(void)
{
    DictObject *dict = mm_alloc_obj(dict);
    INIT_OBJECT_HEAD(dict, &dict_type, 0);
    hashmap_init(&dict->map, _dict_entry_eq_);
    return (Object *)dict;
}

#ifdef __cplusplus
}
#endif
