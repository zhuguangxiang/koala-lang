/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value none_str(Value *self)
{
    Object *s = kl_new_str("none");
    return obj_value(s);
}

TypeObject none_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "NoneType",
    .flags = TP_FLAGS_CLASS,
    .str = none_str,
};

#ifdef __cplusplus
}
#endif
