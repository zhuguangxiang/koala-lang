/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "object.h"
#include "exception.h"

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

#ifdef __cplusplus
}
#endif
