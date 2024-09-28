/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "floatobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject float_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "float",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC | TP_FLAGS_FINAL,
    .size = 0,
};

#ifdef __cplusplus
}
#endif
