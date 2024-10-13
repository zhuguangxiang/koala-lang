/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"
#include "moduleobject.h"
#include "object.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

Object *type_lookup_object(Object *_tp, const char *name, int len)
{
    TypeObject *tp = (TypeObject *)_tp;
    Object *obj = table_find(&tp->map, name, len);
    ASSERT(obj);
    return obj;
}

static Value type_str(Value *self)
{
    TypeObject *tp = as_obj(self);
    ASSERT(IS_TYPE(tp, &type_type));
    const char *s = module_get_name(tp->module);
    Object *ret;
    if (!strcmp(s, "builtin")) {
        ret = kl_new_fmt_str("<class '%s'>", tp->name);
    } else {
        ret = kl_new_fmt_str("<class '%s.%s'>", s, tp->name);
    }
    return obj_value(ret);
}

static Value type_call(Value *self, Value *args, int nargs, Object *names)
{
    TypeObject *type = as_obj(self);

    if (type == &type_type) {
        /* func typeof(obj object) type */
        ASSERT(args && nargs == 1);
        TypeObject *tp = object_typeof(args);
        ASSERT(tp);
        return obj_value(tp);
    }

    Value val;
    if (type->alloc) {
        Object *obj = type->alloc(type);
        val = obj_value(obj);
    } else {
        val = none_value;
    }

    if (type->init) {
        int res = type->init(&val, args, nargs, names);
        if (res < 0) {
            ASSERT(exc_occurred());
            return error_value;
        }
    }

    return val;
}

TypeObject type_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "type",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL | TP_FLAGS_META,
    .str = type_str,
    .call = type_call,
};

#ifdef __cplusplus
}
#endif
