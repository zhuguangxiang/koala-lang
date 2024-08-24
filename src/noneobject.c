/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "strobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _none_str(Value *self)
{
    Object *s = kl_new_str("None");
    return ObjValue(s);
}

TypeObject none_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "none",
    .str = _none_str,
};

#ifdef __cplusplus
}
#endif
