/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject tuple_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Tuple",
};

Object *kl_new_tuple(int size)
{
    int msize = sizeof(TupleObject) + size * sizeof(Value);
    TupleObject *x = gc_alloc(msize);
    INIT_OBJECT_HEAD(x, &tuple_type);
    x->size = size;
    for (int i = 0; i < size; i++) {
        Value *val = (Value *)(x + 1);
        val[i] = NoneValue();
    }
    return (Object *)x;
}

#ifdef __cplusplus
}
#endif
