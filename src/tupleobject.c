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
    Object *tuple = gc_alloc(msize);
    INIT_OBJECT_HEAD(tuple, &tuple_type);
    return tuple;
}

#ifdef __cplusplus
}
#endif
