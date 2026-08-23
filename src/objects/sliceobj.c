/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "sliceobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _slice_index(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    RangeObject *range = (RangeObject *)to_obj(self);
    TValue *value = args + 0;
    int64_t start = range->start.ival;
    int64_t stop = range->stop.ival;
    int64_t step = range->step.ival;

    if (step > 0) {
        if (value->ival < start || value->ival >= stop) {
            return int64_value(-1); // Not found
        }
    } else if (step < 0) {
        if (value->ival > start || value->ival <= stop) {
            return int64_value(-1); // Not found
        }
    } else {
        UNREACHABLE(); // Step cannot be zero, should have been checked during range creation
    }

    if ((value->ival - start) % step != 0) {
        return int64_value(-1); // Not found
    }

    return int64_value((value->ival - start) / step);
}

static TValue _slice_str(TValue *self, TValue *args, int nargs)
{
    RangeObject *range = (RangeObject *)to_obj(self);
    int64_t start = range->start.ival;
    int64_t stop = range->stop.ival;
    int64_t step = range->step.ival;
    Object *sobj = kl_new_fmt_str("range(%ld, %ld, %ld)", start, stop, step);
    return obj_value(sobj);
}

static MethodDef slice_methods[] = {
    { "index", _slice_index },
    { "__str__", _slice_str },
    { NULL },
};

static MemberDef slice_members[] = {
    { "start", M_TYPE_INT, M_OFFSET(RangeObject, start) },
    { "end", M_TYPE_INT, M_OFFSET(RangeObject, end) },
    { "step", M_TYPE_INT, M_OFFSET(RangeObject, step) },
    { NULL },
};

TypeObject slice_type = {
    ._type = &type_type,
    .name = "slice",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = slice_methods,
    .membdefs = slice_members,
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

    if (start > end) {
        panic("slice start cannot be greater than end");
    }

    int msize = sizeof(SliceObject);
    SliceObject *x = mm_alloc(msize);
    INIT_OBJECT_HEAD(x, &slice_type);

    x->start = items[0];
    x->stop = items[1];
    x->step = items[2];

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
