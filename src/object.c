/**
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
extern TypeObject exc_type;
extern TypeObject int_type;
extern TypeObject float_type;

static TypeObject *mapping[] = {
    NULL, NULL, &int_type, NULL, &none_type, NULL,
};

TypeObject *value_type(Value *val)
{
    TypeObject *tp = mapping[val->tag];
    if (tp) return tp;
    if (IS_OBJECT(val)) return OB_TYPE(val->obj);
    return NULL;
}

Object *get_default_kwargs(Value *val)
{
    if (!IS_OBJECT(val)) return NULL;

    TypeObject *tp = value_type(val);
    if (tp->kwds_offset) {
        Object *obj = val->obj;
        return (Object *)((char *)obj + tp->kwds_offset);
    } else {
        return NULL;
    }
}

Value object_call(Object *obj, Value *args, int nargs)
{
    TypeObject *tp = OB_TYPE(obj);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc("'%s' is not callable", tp->name);
        return ErrorValue;
    }

    return call(obj, args, nargs);
}

static Value object_hash(Value *self)
{
    unsigned int v = mem_hash(self, sizeof(Value));
    return IntValue(v);
}

static Value object_compare(Value *self, Value *rhs)
{
    int v = memcmp(self, rhs, sizeof(Value));
    int r;
    if (v > 0) {
        r = 1;
    } else if (v < 0) {
        r = -1;
    } else {
        r = 0;
    }
    return IntValue(r);
}

static Value object_str(Value *self)
{
    TypeObject *tp = value_type(self);
    const char *s = module_get_name(tp->module);
    Object *result = kl_new_fmt_str("<%s.%s object>", s, tp->name);
    return ObjectValue(result);
}

static Value object_get_class(Value *self, Value *args, int nargs)
{
    TypeObject *tp = value_type(self);
    return ObjectValue(tp);
}

static MethodDef object_methods[] = {
    { "__class__", object_get_class },
    { NULL },
};

TypeObject object_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Object",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .size = sizeof(Object),
    .hash = object_hash,
    .cmp = object_compare,
    .str = object_str,
    .methods = object_methods,
};

#ifdef __cplusplus
}
#endif
