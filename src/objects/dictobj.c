/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dictobj.h"
#include "excobj.h"
#include "listobj.h"
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

/* Merge every (key, value) pair yielded by `src` into `dict`, replacing the
 * values of existing keys. Shared by update(), __or__() and __ior__(); `src`
 * is the single argument of those methods and its type Iterable[(K, V)] is
 * enforced by the compiler, so every yielded element is a 2-tuple of (K, V).
 * Returns no_value on success, error_value if the protocol call failed. */
static TValue _dict_merge(DictObject *dict, TValue *src)
{
    /* Dict fast-path: a dict is both Sequence-like (len + __getitem__) and
     * Iterable, but merging must not go through integer-index lookup; iterate
     * the source's entries directly in insertion order. */
    Object *src_obj = to_obj(src);
    if (IS_DICT(src_obj)) {
        DictObject *src_dict = (DictObject *)src_obj;
        HashMapIter it = { 0 };
        while (hashmap_next(&src_dict->map, &it)) {
            DictEntry *entry = (DictEntry *)hashmap_entry(&it);
            _dict_insert(dict, &entry->key, &entry->value);
        }
        return no_value;
    }

    /* Sequence fast-path (list / tuple / str / bytes): use len + __getitem__
     * because Sequence types do not register SLOT_ITER. */
    if (kl_is_seq(src_obj)) {
        TValue len = kl_call_len(src);
        if (is_error(&len)) return error_value;
        size_t m = (size_t)to_int64(&len);
        for (size_t i = 0; i < m; ++i) {
            TValue idx = int64_value((int64_t)i);
            TValue item = kl_call_get_item(src, &idx);
            if (is_error(&item)) return error_value;
            _dict_update_one(dict, &item);
        }
        return no_value;
    }

    /* Generic Iterable path: iter() + next() until next yields nil. */
    if (kl_is_iter(src_obj)) {
        TValue it = kl_call_iter(src, 1);
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

static TValue _dict_update(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    return _dict_merge(dict, &args[0]);
}

/* pub func __bitor__(src Iterable[(K, V)]) dict[K, V]
 *
 * Return a new dict with the mappings of `self` followed by `src`
 * (`self | src`). Values from `src` replace those of duplicate keys; the
 * receiver is left untouched. */
static TValue _dict_bitor(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);

    Object *res = kl_new_dict();
    DictObject *new_dict = (DictObject *)res;

    /* Copy self's mappings first; HashMapIter follows insertion order, so the
     * result keeps self's order and appends keys fresh from `src`. */
    HashMapIter it = { 0 };
    while (hashmap_next(&dict->map, &it)) {
        DictEntry *entry = (DictEntry *)hashmap_entry(&it);
        _dict_insert(new_dict, &entry->key, &entry->value);
    }

    TValue ret = _dict_merge(new_dict, &args[0]);
    if (is_error(&ret)) return ret;
    return obj_value(res);
}

/* pub func __ior__(src Iterable[(K, V)]) dict[K, V]
 *
 * Merge all (key, value) pairs from `src` into the dict in place
 * (`self |= src`), replacing the values of existing keys. Returns the
 * receiver so the compound assignment keeps the same object. */
static TValue _dict_ior(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 1);
    TValue ret = _dict_merge(dict, &args[0]);
    if (is_error(&ret)) return ret;
    return *self;
}

static TValue _dict_keys(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 0);
    Object *list = kl_new_list();
    HashMapIter it = { 0 };
    while (hashmap_next(&dict->map, &it)) {
        DictEntry *entry = (DictEntry *)hashmap_entry(&it);
        kl_list_append(list, entry->key);
    }
    TValue list_tv = obj_value(list);
    return kl_call_iter(&list_tv, 1);
}

static TValue _dict_values(TValue *self, TValue *args, int nargs)
{
    DictObject *dict = SELF_AS(dict_type);
    ASSERT(nargs == 0);
    Object *list = kl_new_list();
    HashMapIter it = { 0 };
    while (hashmap_next(&dict->map, &it)) {
        DictEntry *entry = (DictEntry *)hashmap_entry(&it);
        kl_list_append(list, entry->value);
    }
    TValue list_tv = obj_value(list);
    return kl_call_iter(&list_tv, 1);
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

/* DictKeysIter field layout, must match 'class DictKeysIter' in dict.kl. */
enum { KEYS_ITER_DICT = 0, KEYS_ITER_STEP = 1, KEYS_ITER_POS = 2 };

/* Return the entry at insertion index `pos`, or NULL when out of range. */
static DictEntry *_dict_entry_at(DictObject *dict, int64_t pos)
{
    if (pos < 0 || pos >= (int64_t)dict->map.count) return NULL;
    HashMapIter it = { 0 };
    while (hashmap_next(&dict->map, &it)) {
        if (pos-- == 0) return (DictEntry *)hashmap_entry(&it);
    }
    return NULL;
}

static TValue _keys_iter_has_next(TValue *self, TValue *args, int nargs)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[KEYS_ITER_DICT]);
    return bool_value(inst->fields[KEYS_ITER_POS].ival < (int64_t)dict->map.count);
}

/* Yield the current key and advance by `step`; NULL when exhausted. */
static DictEntry *_keys_iter_advance(TValue *self)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[KEYS_ITER_DICT]);
    TValue *pos = &inst->fields[KEYS_ITER_POS];
    DictEntry *entry = _dict_entry_at(dict, pos->ival);
    if (entry) pos->ival += inst->fields[KEYS_ITER_STEP].ival;
    return entry;
}

static TValue _keys_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _keys_iter_advance(self);
    if (!entry) {
        raise_exc("No more keys in DictKeysIter");
        return error_value;
    }
    return entry->key;
}

static TValue _keys_iter_next(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _keys_iter_advance(self);
    return entry ? entry->key : nil_value;
}

void dict_keys_iter_register(NativeLib *lib)
{
    kl_reg_meth(lib, "DictKeysIter", "has_next", _keys_iter_has_next);
    kl_reg_meth(lib, "DictKeysIter", "next_strict", _keys_iter_next_strict);
    kl_reg_meth(lib, "DictKeysIter", "next", _keys_iter_next);
}

/* DictValuesIter field layout, must match 'class DictValuesIter' in dict.kl. */
enum { VALUES_ITER_DICT = 0, VALUES_ITER_STEP = 1, VALUES_ITER_POS = 2 };

static TValue _values_iter_has_next(TValue *self, TValue *args, int nargs)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[VALUES_ITER_DICT]);
    return bool_value(inst->fields[VALUES_ITER_POS].ival < (int64_t)dict->map.count);
}

/* Yield the current value and advance by `step`; NULL when exhausted. */
static DictEntry *_values_iter_advance(TValue *self)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[VALUES_ITER_DICT]);
    TValue *pos = &inst->fields[VALUES_ITER_POS];
    DictEntry *entry = _dict_entry_at(dict, pos->ival);
    if (entry) pos->ival += inst->fields[VALUES_ITER_STEP].ival;
    return entry;
}

static TValue _values_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _values_iter_advance(self);
    if (!entry) {
        raise_exc("No more values in DictValuesIter");
        return error_value;
    }
    return entry->value;
}

static TValue _values_iter_next(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _values_iter_advance(self);
    return entry ? entry->value : nil_value;
}

void dict_values_iter_register(NativeLib *lib)
{
    kl_reg_meth(lib, "DictValuesIter", "has_next", _values_iter_has_next);
    kl_reg_meth(lib, "DictValuesIter", "next_strict", _values_iter_next_strict);
    kl_reg_meth(lib, "DictValuesIter", "next", _values_iter_next);
}

/* DictViewIter field layout, must match 'class DictViewIter' in dict.kl. */
enum { VIEW_ITER_DICT = 0, VIEW_ITER_STEP = 1, VIEW_ITER_POS = 2 };

static TValue _view_iter_has_next(TValue *self, TValue *args, int nargs)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[VIEW_ITER_DICT]);
    return bool_value(inst->fields[VIEW_ITER_POS].ival < (int64_t)dict->map.count);
}

/* Yield the current entry and advance by `step`; NULL when exhausted. */
static DictEntry *_view_iter_advance(TValue *self)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[VIEW_ITER_DICT]);
    TValue *pos = &inst->fields[VIEW_ITER_POS];
    DictEntry *entry = _dict_entry_at(dict, pos->ival);
    if (entry) pos->ival += inst->fields[VIEW_ITER_STEP].ival;
    return entry;
}

static TValue _view_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _view_iter_advance(self);
    if (!entry) {
        raise_exc("No more items in DictViewIter");
        return error_value;
    }
    TValue items[2] = { entry->key, entry->value };
    return obj_value(kl_new_tuple(items, 2));
}

static TValue _view_iter_next(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _view_iter_advance(self);
    if (!entry) return nil_value;
    TValue items[2] = { entry->key, entry->value };
    return obj_value(kl_new_tuple(items, 2));
}

void dict_view_iter_register(NativeLib *lib)
{
    kl_reg_meth(lib, "DictViewIter", "has_next", _view_iter_has_next);
    kl_reg_meth(lib, "DictViewIter", "next_strict", _view_iter_next_strict);
    kl_reg_meth(lib, "DictViewIter", "next", _view_iter_next);
}

/* DictRevIter field layout, must match 'class DictRevIter' in dict.kl. */
enum { REV_ITER_DICT = 0, REV_ITER_STEP = 1, REV_ITER_POS = 2 };

static TValue _rev_iter_has_next(TValue *self, TValue *args, int nargs)
{
    InstObject *inst = (InstObject *)to_obj(self);
    return bool_value(inst->fields[REV_ITER_POS].ival >= 0);
}

/* Yield the current entry and retreat by `step`; NULL when exhausted. */
static DictEntry *_rev_iter_advance(TValue *self)
{
    InstObject *inst = (InstObject *)to_obj(self);
    DictObject *dict = (DictObject *)to_obj(&inst->fields[REV_ITER_DICT]);
    TValue *pos = &inst->fields[REV_ITER_POS];
    DictEntry *entry = _dict_entry_at(dict, pos->ival);
    if (entry) pos->ival -= inst->fields[REV_ITER_STEP].ival;
    return entry;
}

static TValue _rev_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _rev_iter_advance(self);
    if (!entry) {
        raise_exc("No more items in DictRevIter");
        return error_value;
    }
    TValue items[2] = { entry->key, entry->value };
    return obj_value(kl_new_tuple(items, 2));
}

static TValue _rev_iter_next(TValue *self, TValue *args, int nargs)
{
    DictEntry *entry = _rev_iter_advance(self);
    if (!entry) return nil_value;
    TValue items[2] = { entry->key, entry->value };
    return obj_value(kl_new_tuple(items, 2));
}

void dict_rev_iter_register(NativeLib *lib)
{
    kl_reg_meth(lib, "DictRevIter", "has_next", _rev_iter_has_next);
    kl_reg_meth(lib, "DictRevIter", "next_strict", _rev_iter_next_strict);
    kl_reg_meth(lib, "DictRevIter", "next", _rev_iter_next);
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
    { "__bitor__", _dict_bitor },
    { "__ior__", _dict_ior },
    { "keys", _dict_keys },
    { "values", _dict_values },
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
