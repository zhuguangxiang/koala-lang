/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef collection_methods[] = {
    { "__len__", intf_not_impl, METH_NO_ARGS },
    { "empty", intf_not_impl, METH_NO_ARGS },
    { NULL },
};

static TypeObject *collection_intfs[] = { &iterable_type, NULL };

// clang-format off
TypeObject collection_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Collection",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = collection_methods,
    .intfdefs = collection_intfs,
};
// clang-format on

static MethodDef seq_methods[] = {
    { "__getitem__", intf_not_impl, METH_ONE_ARG },
    { NULL },
};

static TypeObject *seq_intfs[] = { &collection_type, NULL };

// clang-format off
TypeObject sequence_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Sequence",
    .flags = TP_FLAGS_TRAIT,
    .methdefs = seq_methods,
    .intfdefs = seq_intfs,
};
// clang-format on

#ifdef __cplusplus
}
#endif
