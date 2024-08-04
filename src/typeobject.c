/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value type_call(Object *self, Value *args, int nargs, Object *kwds)
{
    return NoneValue;
}

TypeObject type_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "type",
    .call = NULL,
};

TypeObject object_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
};

#ifdef __cplusplus
}
#endif
