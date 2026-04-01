/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef Number_methods[] = {
    { "__add__", NULL },
    { "__sub__", NULL },
    { "__mul__", NULL },
    { "__div__", NULL },
    { "__mod__", NULL },
    { "__lsh__", NULL },
    { "__rsh__", NULL },
    { "__and__", NULL },
    { "__xor__", NULL },
    { "__or__", NULL },
    { NULL },
};

TypeObject Number_type = {
    ._type = &type_type,
    .name = "Number",
    .flags = TP_FLAGS_TRAIT | TP_FLAGS_PUBLIC,
    .methdefs = Number_methods,
};

#ifdef __cplusplus
}
#endif
