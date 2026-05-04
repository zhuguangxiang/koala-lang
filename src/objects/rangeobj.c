/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "rangeobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue range_str(TValue *self, TValue *args, int nargs)
{
    RangeObject *range = (RangeObject *)to_obj(self);
    Object *sobj = kl_new_fmt_str("range(%v, %v, %v)", range->start, range->stop, range->step);
    return obj_value(sobj);
}

static MethodDef range_methods[] = {
    { "__str__", range_str },
    { NULL },
};

TypeObject range_type = {
    ._type = &type_type,
    .name = "range",
    .flags = TP_FLAGS_CLASS,
    .methdefs = range_methods,
};

Object *kl_new_range(TValue *items, int count)
{
    ASSERT(count == 3);
    int msize = sizeof(RangeObject);
    RangeObject *x = mm_alloc(msize);
    INIT_OBJECT_HEAD(x, &range_type);

    x->start = items[0];
    x->stop = items[1];
    x->step = items[2];

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
