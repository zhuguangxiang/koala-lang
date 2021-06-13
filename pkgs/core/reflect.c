/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"
#include "gc/gc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 'Class' type */
static TypeInfo class_type = {
    .name = "Class",
    .flags = TF_CLASS | TF_FINAL,
};

/* `Field` type */
static TypeInfo field_type = {
    .name = "Field",
    .flags = TF_CLASS | TF_FINAL,
};

/* `Method` type */
static TypeInfo method_type = {
    .name = "Method",
    .flags = TF_CLASS | TF_FINAL,
};

/* `Package` type */
static TypeInfo pkg_type = {
    .name = "Package",
    .flags = TF_CLASS | TF_FINAL,
};

typedef struct _ClassObject ClassObject;

struct _ClassObject {
    OBJECT_HEAD
    TypeInfo *typeinfo;
};

void init_reflect_types(void)
{
    // clang-format off
    MethodDef class_methods[] = {
        METHOD_DEF("__name__",   nil, "s",          nil),
        METHOD_DEF("__pkg__",    nil, "LClass;",    nil),
        METHOD_DEF("__mbrs__",   nil, "LArray(A);", nil),
        METHOD_DEF("get_field",  "s", "LField;",    nil),
        METHOD_DEF("get_method", "s", "LMethod;",   nil),
        METHOD_DEF("__str__",    nil, "s",          nil),
    };
    // clang-format on
}

Object *class_new(TypeInfo *type)
{
    ClassObject *clazz = gc_alloc(sizeof(*clazz), nil);
    clazz->type = &class_type;
    clazz->typeinfo = type;
    return (Object *)clazz;
}

#ifdef __cplusplus
}
#endif
