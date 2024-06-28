/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "strobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject str_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "str",
};

#ifdef __cplusplus
}
#endif
