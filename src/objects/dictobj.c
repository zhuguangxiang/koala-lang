/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dictobj.h"
#include "excobj.h"

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
    TValue *key = &args[0];
    TValue *value = &args[1];

    DictEntry key_entry = { .key = *key };
    unsigned int hash = kl_call_hash(key);
    hashmap_entry_init(&key_entry, hash);
    DictEntry *entry = hashmap_get(&dict->map, &key_entry);
    if (entry) {
        entry->value = *value;
    } else {
        // TODO: need to update order-list?
        entry = mm_alloc(sizeof(DictEntry));
        entry->key = *key;
        entry->value = *value;
        hashmap_entry_init(entry, hash);
        hashmap_put(&dict->map, entry);
    }
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

static MethodDef dict_methods[] = {
    { "__init__", _dict_init },         { "len", _dict_len },
    { "empty", _dict_empty },           { "fmt", _dict_fmt },
    { "__setitem__", _dict_set_item },  { "__getitem__", _dict_get_item },
    { "__contains__", _dict_contains }, { NULL },
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
