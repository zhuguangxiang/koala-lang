/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value bool_str(Value *self)
{
    Object *s = kl_new_fmt_str("%s", self->bval ? "true" : "false");
    return object_value(s);
}

TypeObject bool_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "bool",
    .flags = TP_FLAGS_CLASS,
    .str = bool_str,
};

#ifdef __cplusplus
}
#endif
