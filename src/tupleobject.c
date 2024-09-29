/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobject.h"
#include "shadowstack.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject tuple_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Tuple",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_FINAL | TP_FLAGS_PUBLIC,
    .size = sizeof(TupleObject),
    .str = tuple_str,
};

Object *kl_new_tuple(int size)
{
    TupleObject *x = gc_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &tuple_type);
    x->start = 0;
    x->stop = size;

    init_gc_stack_one(x);

    GcArrayObject *arr = gc_alloc_array(GC_KIND_ARRAY_VALUE, size);

    Value *values = (Value *)(arr + 1);
    for (int i = 0; i < size; i++) {
        Value *val = (Value *)(values + 1);
        val[i] = NoneValue;
    }

    fini_gc_stack();

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
