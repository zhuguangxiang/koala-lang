/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobject.h"
#include "gc.h"
#include "shadowstack.h"

#ifdef __cplusplus
extern "C" {
#endif

static void tuple_gc_mark(TupleObject *obj, Queue *que)
{
    if (obj->array) gc_mark_array(obj->array, que);
}

static Value tuple_str(Value *self) { return none_value; }

// clang-format off
TypeObject tuple_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "tuple",
    .flags = TP_FLAGS_CLASS,
    .str = tuple_str,
    .mark = (MarkFunc)tuple_gc_mark,
};
// clang-format on

Object *kl_new_tuple(size_t size)
{
    TupleObject *x = gc_alloc_obj(x);
    INIT_OBJECT_HEAD(x, &tuple_type);
    x->size = size;

    kl_gc_protect(x);

    void *data = gc_alloc_value_array(size);
    x->array = data;

    Value *values = (Value *)(x->array);
    for (size_t i = 0; i < size; i++) {
        Value *val = (Value *)(values + i);
        *val = none_value;
    }

    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
