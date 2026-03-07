/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef iterable_methods[] = {
    { "__iter__", intf_not_impl, METH_NO_ARGS },
    { NULL },
};

TypeObject iterable_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterable",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = iterable_methods,
};

static MethodDef iterator_methods[] = {
    { "__has_next__", intf_not_impl, METH_NO_ARGS },
    { "__next__", intf_not_impl, METH_NO_ARGS },
    { NULL },
};

static TypeObject *iterator_traits[] = { &iterable_type, NULL };

// clang-format off
TypeObject iterator_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterator",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = iterator_methods,
    .intfdefs = iterator_traits,
};
// clang-format on

#ifdef __cplusplus
}
#endif
