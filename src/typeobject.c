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

static Value type_str(Value *self)
{
    ASSERT(IS_OBJECT(self));
    TypeObject *tp = self->obj;
    ASSERT(IS_TYPE(tp, &type_type));
    const char *s = module_get_name(tp->module);
    Object *result = kl_new_fmt_str("<class %s.%s>", s, tp->name);
    return object_value(result);
}

static Value type_call(Value *self, Value *args, int nargs, Object *names)
{
    TypeObject *type = value_as_object(self);

    if (type == &type_type) {
        /* func typeof(obj object) type */
        ASSERT(args && nargs == 1);
        TypeObject *tp = object_type(args);
        ASSERT(tp);
        return object_value(tp);
    }

    Value val;
    if (type->alloc) {
        Object *obj = type->alloc(type);
        val = object_value(obj);
    } else {
        val = (Value){ 0 };
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
