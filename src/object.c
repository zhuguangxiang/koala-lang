/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "exception.h"
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *mapping[] = {
    &none_type, &int_type, &float_type, NULL, &exc_type,
};

TypeObject *object_typeof(Value *val)
{
    TypeObject *tp = mapping[val->tag];
    if (tp) return tp;
    return OB_TYPE(to_obj(val));
}

Value object_call(Value *self, Value *args, int nargs, Object *names)
{
    TypeObject *tp = object_typeof(self);
    ASSERT(tp);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc_fmt("'%s' is not callable", tp->name);
        return error_value;
    }

    return call(self, args, nargs, names);
}

Object *kl_lookup_method(Value *obj, const char *fname)
{
    TypeObject *tp = object_typeof(obj);
    Object *fn = hashmap_get(&tp->map, NULL);
    return fn;
}

Value methodsite_call(MethodSite *ms, Value *args, int nargs, Object *names)
{
    TypeObject *tp = object_typeof(args);
    ASSERT(tp);
    if (tp != ms->type) {
        Object *meth = kl_lookup_method(args, ms->fname);
        ASSERT(meth);
        ms->type = tp;
        ms->method = meth;
    }
    Value v = obj_value(ms->method);
    return object_call(&v, args, nargs, names);
}

static int get_name_index(Object *names, const char *name)
{
    Value *items = TUPLE_ITEMS(names);
    int len = TUPLE_LEN(names);

    for (int i = 0; i < len; i++) {
        Object *sobj = as_obj(items + i);
        ASSERT(IS_STR(sobj));
        const char *s = STR_BUF(sobj);
        if (!strcmp(s, name)) {
            return i;
        }
    }

    return -1;
}

/**
 * Parse optional keyword arguments of this function.
 *
 * @return 0 successful, -1 error
 *
 * @param args The base pointer of arguments passed to this function.
 * @param nargs The number of positional arguments passed to this function.
 * @param names The tuple of keyword arguments' names passed to this function.
 * @param npos The number of positional arguments defined by this function.
 * @param kws The string array of acceptable keyword arguments' names defined by this
 * function.
 *
 * @note
 * The value of `nargs - npos` is the number of keyword arguments which are passed by
 * positional arguments.
 *
 * It will be checked by compiler if there are arguments both have positional values and
 * keyword values.
 */
int kl_parse_kwargs(Value *args, int nargs, Object *names, int npos, const char **kws,
                    ...)
{
    ASSERT(nargs >= npos);
    Value *v;
    va_list va_args;
    va_start(va_args, kws);

    // Parse keyword arguments which are passed by position.
    for (int i = npos; i < nargs; i++) {
        v = va_arg(va_args, Value *);
        *v = *(args + i);
        ++kws;
    }

    // Parse keyword arguments which are passed by keyword.
    if (names) {
        ASSERT(IS_TUPLE(names));
        const char *kw;
        while ((kw = *kws)) {
            int i = get_name_index(names, kw);
            v = va_arg(va_args, Value *);
            if (i >= 0) {
                *v = *(args + nargs + i);
            }
            ++kws;
        }
    }

    va_end(va_args);
    return 0;
}

Value object_str(Value *self)
{
    TypeObject *tp = object_typeof(self);
    ASSERT(tp->str);
    return tp->str(self);
}

static int _table_func_equal_(void *e1, void *e2)
{
    SymbolEntry *n1 = e1;
    SymbolEntry *n2 = e2;
    if (n1->len != n2->len) return 0;
    if (!strncmp(n1->key, n2->key, n1->len)) return 1;
    return 0;
}

void table_add_object(HashMap *map, const char *name, Object *obj)
{
    SymbolEntry *e = mm_alloc_obj_fast(e);
    unsigned int hash = str_hash(name);
    hashmap_entry_init(e, hash);
    e->key = name;
    e->len = strlen(name);
    e->obj = obj;
    int r = hashmap_put_absent(map, e);
    ASSERT(!r);
}

Object *table_find(HashMap *map, const char *name, int len)
{
    SymbolEntry entry = { .key = name, .len = len };
    unsigned int hash = mem_hash(name, len);
    hashmap_entry_init(&entry, hash);
    SymbolEntry *found = hashmap_get(map, &entry);
    return found ? found->obj : NULL;
}

void init_symbol_table(HashMap *map) { hashmap_init(map, _table_func_equal_); }

static Value base_hash(Value *self)
{
    unsigned int v = mem_hash(self, sizeof(Value));
    return int_value(v);
}

static Value base_compare(Value *self, Value *rhs)
{
    int v = memcmp(self, rhs, sizeof(Value));
    int r = _compare_result(v);
    return int_value(r);
}

static Value base_str(Value *self)
{
    TypeObject *tp = object_typeof(self);
    Object *res = kl_new_fmt_str("<%s object>", tp->name);
    return obj_value(res);
}

static MethodDef base_methods[] = {
    { "__hash__", base_hash, METH_NO_ARGS, "", "i" },
    { "__cmp__", base_compare, METH_ONE_ARG, "O", "b" },
    { "__str__", base_str, METH_NO_ARGS, "", "s" },
    { NULL },
};

TypeObject base_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .hash = base_hash,
    .cmp = base_compare,
    .str = base_str,
    .methods = base_methods,
};

static Value none_str(Value *self)
{
    Object *s = kl_new_str("none");
    return obj_value(s);
}

TypeObject none_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "NoneType",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .str = none_str,
};

static Value undef_str(Value *self)
{
    Object *s = kl_new_str("undef");
    return obj_value(s);
}

#ifdef __cplusplus
}
#endif
