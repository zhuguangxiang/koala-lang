/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef Iterable_methods[] = {
    { "__iter__", intf_not_impl, METH_NO_ARGS },
    { NULL },
};

TypeObject Iterable_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterable",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = Iterable_methods,
};

static MethodDef Iterator_methods[] = {
    { "__has_next__", intf_not_impl, METH_NO_ARGS },
    { "__next__", intf_not_impl, METH_NO_ARGS },
    { NULL },
};

// clang-format off
TypeObject Iterator_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterator",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = Iterator_methods,
};
// clang-format on

#ifdef __cplusplus
}
#endif
