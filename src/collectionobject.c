/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef Collection_methods[] = {
    { "__len__", intf_not_impl, METH_NO_ARGS },
    { "empty", intf_not_impl, METH_NO_ARGS },
    { NULL },
};

static BaseDef Collection_bases[] = {
    { &Iterable_type },
    { NULL },
};

// clang-format off
TypeObject Collection_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Collection",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = Collection_methods,
    .basedefs = Collection_bases,
};
// clang-format on

static MethodDef Sequence_methods[] = {
    { "__getitem__", intf_not_impl, METH_ONE_ARG },
    { NULL },
};

static BaseDef Sequence_bases[] = {
    { &Collection_type },
    { NULL },
};

// clang-format off
TypeObject Sequence_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name     = "Sequence",
    .flags    = TP_FLAGS_TRAIT,
    .basedefs = Sequence_bases,
    .methdefs = Sequence_methods,
};
// clang-format on

static MethodDef MutableSequence_methods[] = {
    { "__getitem__", intf_not_impl_arg, METH_ONE_ARG },
    { NULL },
};

static BaseDef MutableSequence_bases[] = {
    { &Sequence_type },
    { NULL },
};

// clang-format off
TypeObject MutableSequence_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "MutableSequence",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = MutableSequence_methods,
    .basedefs = MutableSequence_bases,
};
// clang-format on

#ifdef __cplusplus
}
#endif
