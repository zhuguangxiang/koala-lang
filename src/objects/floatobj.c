/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject float_type = {
    ._type = &type_type,
    .name = "float",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
};

#ifdef __cplusplus
}
#endif
