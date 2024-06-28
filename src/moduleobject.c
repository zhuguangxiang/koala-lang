/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "moduleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject module_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "module",
};

#ifdef __cplusplus
}
#endif
