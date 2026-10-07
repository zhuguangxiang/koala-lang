/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "rangeobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

int kl_range_len(RangeObject *range, int64_t *out)
{
    int64_t start = to_int64(&range->start);
    int64_t end = to_int64(&range->end);
    int64_t step = to_int64(&range->step);

    if (step == 0) {
        raise_exc("range step cannot be zero");
        return 0;
    }

    bool forward = (step > 0);

    if (forward) {
        if (start >= end) {
            *out = 0;
            return 1;
        }

        *out = (end - start + step - 1) / step;
    } else {
        if (start <= end) {
            *out = 0;
            return 1;
        }

        *out = (start - end - step - 1) / -step;
    }

    return 1;
}

int kl_range_index(RangeObject *range, int64_t index, int64_t *out)
{
    int64_t len;

    if (!kl_range_len(range, &len)) return 0;

    if (index < 0 || index >= len) {
        raise_exc("range index out of range");
        return 0;
    }

    int64_t start = to_int64(&range->start);
    int64_t step = to_int64(&range->step);

    *out = start + index * step;
    return 1;
}

static TValue _range_len(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    RangeObject *range = SELF_AS(range_type);
    int64_t start = to_int64(&range->start);
    int64_t end = to_int64(&range->end);
    int64_t step = to_int64(&range->step);

    if (step > 0) {
        return int64_value((end - start + step - 1) / step);
    } else if (step < 0) {
        return int64_value((start - end - step - 1) / (-step));
    } else {
        raise_exc("range step cannot be zero");
        return error_value;
    }
}

static TValue _range_empty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    RangeObject *range = SELF_AS(range_type);
    int64_t start = to_int64(&range->start);
    int64_t end = to_int64(&range->end);
    int64_t step = to_int64(&range->step);

    if (step > 0) {
        return bool_value(start >= end);
    } else if (step < 0) {
        return bool_value(start <= end);
    } else {
        raise_exc("range step cannot be zero");
        return error_value;
    }
}

static TValue _range_index(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    RangeObject *range = SELF_AS(range_type);
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

static TValue _range_get_item(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    RangeObject *range = SELF_AS(range_type);
    int64_t start = to_int64(&range->start);
    int64_t end = to_int64(&range->end);
    int64_t step = to_int64(&range->step);
    int64_t index = to_int64(&args[0]);

    TValue len_val = _range_len(self, NULL, 0);
    int64_t len = to_int64(&len_val);
    if (index < 0) {
        index += len;
    }

    if (index < 0 || index >= len) {
        raise_exc("range index out of range");
        return error_value;
    }

    return int64_value(start + index * step);
}

static MethodDef range_methods[] = {
    { "len", _range_len },
    { "empty", _range_empty },
    { "__getitem__", _range_get_item },
    { "index", _range_index },
    { NULL },
};

TypeObject range_type = {
    ._type = &type_type,
    .name = "range",
    .flags = TP_FLAGS_CLASS,
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
