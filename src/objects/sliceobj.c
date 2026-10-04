/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject slice_type = {
    ._type = &type_type,
    .name = "slice",
    .flags = TP_FLAGS_CLASS,
};

Object *kl_new_slice(TValue *items)
{
    int64_t start = to_int64(&items[0]);
    int64_t end = to_int64(&items[1]);
    int64_t step = to_int64(&items[2]);

    int msize = sizeof(SliceObject);
    SliceObject *x = mm_alloc(msize);
    INIT_OBJECT_HEAD(x, &slice_type, 3);

    x->start = items[0];
    x->end = items[1];
    x->step = items[2];

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
