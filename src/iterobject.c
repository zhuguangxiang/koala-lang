/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static MethodDef iterable_methods[] = {
    { "iter", NULL, METH_NO_ARGS, "", "LIterator<T>" },
    { NULL },
};

TraitObject iterable_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterable",
    .flags = TP_FLAGS_TRAIT | TP_FLAGS_PUBLIC,
    .methods = iterable_methods,
};

static MethodDef iterator_methods[] = {
    { "next", NULL, METH_NO_ARGS, "", "<T>" },
    { NULL },
};

static TraitObject *iterator_traits[] = { &iterable_type, NULL };

TraitObject iterator_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Iterator",
    .flags = TP_FLAGS_TRAIT | TP_FLAGS_PUBLIC,
    .methods = iterator_methods,
    .traits = iterator_traits,
};

#ifdef __cplusplus
}
#endif
