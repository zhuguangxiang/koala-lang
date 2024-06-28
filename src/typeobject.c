/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject type_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "type",
    .offset_kwargs = offsetof(TypeObject, kwargs),
};

TypeObject object_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "object",
};

#ifdef __cplusplus
}
#endif
