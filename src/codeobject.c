/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "codeobject.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject code_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "code",
};

#ifdef __cplusplus
}
#endif
