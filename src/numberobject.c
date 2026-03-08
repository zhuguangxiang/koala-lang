/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef Number_methods[] = {
    { "__add__", intf_not_impl_arg, METH_ONE_ARG },
    { NULL },
};

TypeObject Number_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name     = "Number",
    .flags    = TP_FLAGS_TRAIT,
    .methdefs = Number_methods,
};

#ifdef __cplusplus
}
#endif
