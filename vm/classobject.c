/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "gc/gc.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ClassObject {
    OBJECT_HEAD
    ObjectRef type;
} ClassObject, *ClassObjectRef;

ObjectRef class_new(ObjectRef type)
{
    ClassObjectRef clazz = gc_alloc(sizeof(ClassObject), NULL);
    clazz->type = type;
    return (ObjectRef)clazz;
}

#ifdef __cplusplus
}
#endif
