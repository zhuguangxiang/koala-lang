/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"
#include "moduleobject.h"
#include "object.h"
#include "strobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value type_str(Value *self)
{
    ASSERT(IS_OBJ(self));
    TypeObject *tp = self->obj;
    ASSERT(IS_TYPE(tp, &type_type));
    const char *s = module_get_name(tp->module);
    Object *result = kl_new_fmt_str("<class %s.%s>", s, tp->name);
    return ObjValue(result);
}

static Value type_call(TypeObject *type, Value *args, int nargs)
{
    if (type == &type_type) {
        ASSERT(args);
        TypeObject *tp = object_type(args);
        ASSERT(tp);
        return ObjValue(tp);
    }

    Value val;
    if (type->size != 0) {
        Object *obj = gc_alloc(type->size);
        INIT_OBJECT_HEAD(obj, type);
        val = ObjValue(obj);
    } else {
        val = (Value){ 0 };
    }

    if (type->init) {
        int res = type->init(&val, args, nargs);
        if (res < 0) {
            ASSERT(exc_occurred());
            return ErrorValue;
        }
    }

    return val;
}

TypeObject type_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "type",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL | TP_FLAGS_META,
    .size = sizeof(TypeObject),
    .str = type_str,
    .call = (CallFunc)type_call,
};

#ifdef __cplusplus
}
#endif
