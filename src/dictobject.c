/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "dictobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _dict_call(Value *self, Value *args, int nargs, Object *kwargs) {}

TypeObject dict_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "dict",
    .call = _dict_call,
};

#ifdef __cplusplus
}
#endif
