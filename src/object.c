/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "object.h"
#include "exception.h"
#include "moduleobject.h"
#include "strobject.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject none_type;
extern TypeObject int_type;
extern TypeObject float_type;
extern TypeObject exc_type;

static TypeObject *mapping[] = {
    &none_type, NULL, &int_type, &float_type, &exc_type,
};

TypeObject *object_type(Value *val)
{
    TypeObject *tp = mapping[val->tag];
    if (tp) return tp;
    if (IS_OBJECT(val)) return OB_TYPE(val->obj);
    return NULL;
}

Object *object_generic_alloc(TypeObject *tp)
{
    ASSERT(tp->size > 0);
    Object *obj = gc_alloc(tp->size);
    INIT_OBJECT_HEAD(obj, tp);
    return obj;
}

Value object_call(Value *self, Value *args, int nargs, KeywordMap *kwargs)
{
    TypeObject *tp = object_type(self);
    ASSERT(tp);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc("'%s' is not callable", tp->name);
        return ErrorValue;
    }

    return call(self, args, nargs, kwargs);
}

Object *object_lookup_method(Value *obj, const char *fname)
{
    TypeObject *tp = object_type(obj);
    Object *fn = hashmap_get(tp->vtbl, NULL);
    return fn;
}

Value object_call_site_method(SiteMethod *sm, Value *args, int nargs, KeywordMap *kwargs)
{
    TypeObject *tp = object_type(args);
    ASSERT(tp);
    if (tp != sm->type) {
        Object *meth = object_lookup_method(args, sm->fname);
        ASSERT(meth);
        sm->type = tp;
        sm->method = meth;
    }
    Value v = ObjectValue(sm->method);
    return object_call(&v, args, nargs, kwargs);
}

int kl_parse_names(Value *args, Object *names, const char **kws, ...)
{
    int i = 0;
    Value *item;
    tuple_foreach(item, names) {
        Object *sobj = value_as_object(item);
        ASSERT(IS_STR(sobj));
        const char *s = STR_BUF(sobj);

        int j = 0;
        while (*kws) {
            const char *kw = *kws;
            if (!strcmp(s, kw)) {
                v = args + i;
            }
            ++kws;
            ++j;
        }

        ++i;

        if (!strcmp(s, "sep")) {
        } else if (!strcmp(s, "end")) {
        } else if (!strcmp(s, "file")) {
        } else {
            unreachable();
        }
    }
}

static Value object_hash(Value *self)
{
    unsigned int v = mem_hash(self, sizeof(Value));
    return IntValue(v);
}

static Value object_compare(Value *self, Value *rhs)
{
    int v = memcmp(self, rhs, sizeof(Value));
    int r = _compare_result(v);
    return IntValue(r);
}

static Value object_str(Value *self)
{
    TypeObject *tp = object_type(self);
    Object *res = kl_new_fmt_str("<%s object>", tp->name);
    return ObjectValue(res);
}

static Value object_hash_code(Value *self) { return object_hash(self); }

static Value object_equals(Value *self, Value *rhs)
{
    int v = memcmp(self, rhs, sizeof(Value));
    return (v == 0) ? IntValue(1) : IntValue(0);
}

static Value object_to_str(Value *self) { return object_str(self); }

static MethodDef object_methods[] = {
    { "hash_code", object_hash_code, METH_NO_ARGS, "", "i" },
    { "equals", object_equals, METH_ONE_ARG, "Lbuiltin.object;", "b" },
    { "to_str", object_to_str, METH_NO_ARGS, "", "s" },
    { NULL },
};

TypeObject base_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .size = sizeof(Object),
    .hash = object_hash,
    .cmp = object_compare,
    .str = object_str,
    .alloc = object_generic_alloc,
    .methods = object_methods,
};

#ifdef __cplusplus
}
#endif
