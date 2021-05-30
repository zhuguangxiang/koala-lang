/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "vm/object.h"

typedef struct _ArrayObject {
    OBJECT_HEAD
    GCArrayRef arr;
    uint32 start;
    uint32 end;
} ArrayObject, *ArrayObjectRef;

RefVal _K$array$new(int size)
{
    ArrayObjectRef arr = gc_alloc(sizeof(*arr));
    void *gc_arr = gc_array_alloc(size, 32);
}

ValueRef _K$array$length(ObjectRef self)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    return arr->end - arr->start;
}
