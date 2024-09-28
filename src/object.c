/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "object.h"
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
    NULL, NULL, &int_type, &float_type, &none_type, &exc_type,
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
    TypeObject *tp = &_object_type;
    Object *obj = value_as_object(self);
    const char *s = module_get_name(tp->module);
    Object *result = kl_new_fmt_str("<%s.%s object at %p>", s, tp->name, obj);
    return ObjectValue(result);
}

TypeObject _object_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .size = sizeof(Object),
    .hash = object_hash,
    .cmp = object_compare,
    .str = object_str,
    .alloc = object_generic_alloc,
};

#ifdef __cplusplus
}
#endif
