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

/* 'Class' type */
static TypeInfo class_type = {
    .name = "Class",
    .flags = TF_CLASS | TF_PUB | TF_FINAL,
};

/* `Field` type */
static TypeInfo field_type = {
    .name = "Field",
    .flags = TF_CLASS | TF_PUB | TF_FINAL,
};

/* `Method` type */
static TypeInfo method_type = {
    .name = "Method",
    .flags = TF_CLASS | TF_PUB | TF_FINAL,
};

typedef struct _ClassObject ClassObject;
typedef struct _MemberObject MemberObject;

struct _ClassObject {
    OBJECT_HEAD
    TypeInfo *typeinfo;
};

struct _MemberObject {
    OBJECT_HEAD
    MemberInfo *mbr;
};

void init_class_type(void)
{
    // clang-format off
    MethodDef class_methods[] = {
        METHOD_DEF("__name__", NULL, "s",  class_get_name),
        { "__module__", NULL, "LClass;",         class_get_module },
        { "__mbrs__",   NULL, "LArray(A);",       class_get_mbrs   },
        { "get_field",  "s",  "LField;",          class_get_field  },
        { "get_method", "s",  "LMethod;",         class_get_method },
        { "__str__",  NULL, "s",                class_to_string  },
    };
    // clang-format on
}

Object *class_new(TypeInfo *type)
{
    ClassObject *clazz = gc_alloc(sizeof(*clazz), NULL);
    clazz->type = &class_type;
    clazz->typeinfo = type;
    return (Object *)clazz;
}

#ifdef __cplusplus
}
#endif
