/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "object.h"

/*
    pub final class Array<T> {

        pub func length() int32 {}
    }
*/

typedef struct _ArrayObject {
    OBJECT_HEAD
    GcArrayRef arr;
    int32 start;
    int32 end;
} ArrayObject, *ArrayObjectRef;

static TypeObjectRef array_type;

ObjectRef array_new(int32 size, int8 ref)
{
    ArrayObjectRef arr;
    GcTraceBlock __gc_trace__ = { &arr };

    arr = gc_alloc(array_type);
    arr->arr = gc_array_alloc(size, ref);
    GC_POP();

    return arr;
}

int32 array_length(ObjectRef self)
{
    ArrayObjectRef arr = (ArrayObjectRef)self;
    return arr->end - arr->start;
}

static MethodDef array_methods[] = {
    { "length", NULL, "i32", array_length },
    { NULL },
};
