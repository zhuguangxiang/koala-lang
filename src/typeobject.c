/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value type_call(Object *self, Value *args, int nargs, Object *kwargs)
{
    return NoneValue;
}

TypeObject type_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "type",
    .kwargs_offset = offsetof(TypeObject, kwargs),
    .call = type_call,
};

TypeObject object_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
};

#ifdef __cplusplus
}
#endif
