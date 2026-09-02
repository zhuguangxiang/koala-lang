/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _slice_str(TValue *self, TValue *args, int nargs)
{
    SliceObject *slice = SELF_AS(slice_type);
    int64_t start = slice->start.ival;
    int64_t end = slice->end.ival;
    int64_t step = slice->step.ival;
    Object *sobj = kl_new_fmt_str("slice(%ld, %ld, %ld)", start, end, step);
    return obj_value(sobj);
}

static MethodDef slice_methods[] = {
    { "__str__", _slice_str },
    { NULL },
};

TypeObject slice_type = {
    ._type = &type_type,
    .name = "slice",
    .flags = TP_FLAGS_CLASS,
    .methdefs = slice_methods,
};

Object *kl_new_slice(TValue *items)
{
    int64_t start = to_int64(&items[0]);
    int64_t end = to_int64(&items[1]);
    int64_t step = to_int64(&items[2]);

    if (start < 0) {
        panic("slice start cannot be negative");
    }

    if (end < -1) {
        panic("slice end cannot be less than -1");
    }

    if (step <= 0) {
        panic("slice step cannot be zero");
    }

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
