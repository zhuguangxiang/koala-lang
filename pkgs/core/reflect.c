/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"
#include "gc/gc.h"
#include "util/buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 'Class' type */
static TypeInfo class_type = {
    .name = "Class",
    .flags = TF_CLASS | TF_FINAL,
};

typedef struct _ClassObj ClassObj;

struct _ClassObj {
    VirtTable *vtbl;
    TypeInfo *type;
};

uintptr class_new(TypeInfo *type)
{
    ClassObj *clazz = gc_alloc(sizeof(*clazz), nil);
    clazz->vtbl = class_type.vtbl[0];
    clazz->type = type;
    return (uintptr)clazz;
}

static uintptr class_name(uintptr self)
{
    ClassObj *obj = (ClassObj *)self;
    return string_new(obj->type->name);
}

static uintptr class_tostr(uintptr self)
{
    ClassObj *obj = (ClassObj *)self;
    BUF(sbuf);
    buf_write_str(&sbuf, "class ");
    buf_write_str(&sbuf, obj->type->name);
    uintptr sobj = string_new(BUF_STR(sbuf));
    FINI_BUF(sbuf);
    return sobj;
}

static void init_class_type(void)
{
    MethodDef class_methods[] = {
        /* clang-format off */
        METHOD_DEF("__name__",   nil, "s",          class_name),
        METHOD_DEF("__pkg__",    nil, "LClass;",    nil),
        METHOD_DEF("__mbrs__",   nil, "LArray(A);", nil),
        METHOD_DEF("get_field",  "s", "LField;",    nil),
        METHOD_DEF("get_method", "s", "LMethod;",   nil),
        METHOD_DEF("__str__",    nil, "s",          class_tostr),
        /* clang-format on */
    };

    type_add_methdefs(&class_type, class_methods);
    type_ready(&class_type);
    type_show(&class_type);
    pkg_add_type("/", &class_type);
}

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

void init_reflect_types(void)
{
    init_class_type();
}

#ifdef __cplusplus
}
#endif
