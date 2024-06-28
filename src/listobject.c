/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "listobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject list_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "list",
};

#ifdef __cplusplus
}
#endif
