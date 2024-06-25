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
    .name = "Class",
    .objsize = sizeof(TypeObject),
};

TypeObject object_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Object",
    .objsize = sizeof(Object),
};

#ifdef __cplusplus
}
#endif
