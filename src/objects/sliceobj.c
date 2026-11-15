/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "gc.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject slice_type = {
    ._type = &type_type,
    .name = "slice",
    .flags = TP_FLAGS_CLASS,
};

static Object *slice_init(SliceObject *x, TValue *items)
{
    INIT_OBJECT_HEAD(x, &slice_type, 3);

    x->start = items[0];
    x->end = items[1];
    x->step = items[2];

    return (Object *)x;
}

Object *kl_new_slice(TValue *items)
{
    SliceObject *x = gc_alloc_obj(x);
    return slice_init(x, items);
}

/* a perm slice for the const pool */
Object *kl_new_perm_slice(TValue *items)
{
    SliceObject *x = gc_alloc_perm_obj(x, 0);
    return slice_init(x, items);
}

#ifdef __cplusplus
}
#endif
