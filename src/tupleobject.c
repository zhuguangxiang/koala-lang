/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobject.h"
#include "shadowstack.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value tuple_str(Value *self)
{
    TupleObject *obj = as_obj(self);
    Value r = none_value;
    return r;
}

TypeObject tuple_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "tuple",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_FINAL | TP_FLAGS_PUBLIC,
    .str = tuple_str,
};

Object *kl_new_tuple(int size)
{
    TupleObject *x = gc_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &tuple_type);
    x->start = 0;
    x->stop = size;

    init_gc_stack_push(1, x);
    GcArrayObject *arr = gc_alloc_array(GC_KIND_ARRAY_VALUE, size);
    fini_gc_stack();
    x->array = arr;

    Value *values = (Value *)(arr + 1);
    for (int i = 0; i < size; i++) {
        Value *val = (Value *)(values + i);
        *val = none_value;
    }

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
