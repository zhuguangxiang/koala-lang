/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "rangeobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue kl_range_index(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    RangeObject *range = (RangeObject *)to_obj(self);
    TValue *value = args + 0;
    int64_t start = to_int64(&range->start);
    int64_t end = to_int64(&range->end);
    int64_t step = to_int64(&range->step);

    if (step > 0) {
        if (value->ival < start || value->ival >= end) {
            return int64_value(-1); // Not found
        }
    } else if (step < 0) {
        if (value->ival > start || value->ival <= end) {
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

static TValue kl_range_str(TValue *self, TValue *args, int nargs)
{
    RangeObject *range = (RangeObject *)to_obj(self);
    int64_t start = to_int64(&range->start);
    int64_t end = to_int64(&range->end);
    int64_t step = to_int64(&range->step);
    Object *sobj = kl_new_fmt_str("range(%ld, %ld, %ld)", start, end, step);
    return obj_value(sobj);
}

static MethodDef range_methods[] = {
    { "index", kl_range_index },
    { "__str__", kl_range_str },
    { NULL },
};

TypeObject range_type = {
    ._type = &type_type,
    .name = "range",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = range_methods,
};

Object *kl_new_range(TValue *items)
{
    int64_t step = to_int64(&items[2]);
    if (step == 0) {
        panic("range step cannot be zero");
    }

    int msize = sizeof(RangeObject);
    RangeObject *x = mm_alloc(msize);
    INIT_OBJECT_HEAD(x, &range_type, 3);

    x->start = items[0];
    x->end = items[1];
    x->step = items[2];

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
