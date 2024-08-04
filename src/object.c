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

TypeObject *value_type(Value *val)
{
    TypeObject *tp = NULL;
    switch (val->tag) {
        case VAL_TAG_OBJ:
            tp = OB_TYPE(val->obj);
            break;
        case VAL_TAG_NONE:
            tp = &none_type;
            break;
        case VAL_TAG_ERR:
            // tp = &exc_type;
            break;
        case VAL_TAG_INT:
            tp = &int_type;
            break;
        case VAL_TAG_FLT:
            // tp = &float_type;
            break;
        default:
            UNREACHABLE();
            break;
    }
    return tp;
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

Value object_call(Object *obj, Value *args, int nargs, Object *kwargs)
{
    TypeObject *tp = OB_TYPE(obj);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc("'%s' is not callable", tp->name);
        return ErrorValue;
    }

    return call(obj, args, nargs, kwargs);
}

#ifdef __cplusplus
}
#endif
