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

/*
final class Class {
    func __init__(obj Any) {}
}
*/

static TypeInfo class_type = {
    .name = "Class",
    .flags = TF_CLASS | TF_FINAL,
};

typedef struct _ClassObj ClassObj;

struct _ClassObj {
    VTable *vtbl;
    TypeInfo *type;
};

objref class_new(objref obj)
{
    TypeInfo *type = __GET_TYPE(obj);
    ClassObj *cobj = gc_alloc(sizeof(*cobj), nil);
    cobj->vtbl = class_type.vtbl[0];
    cobj->type = type;
    return (objref)cobj;
}

static objref class_name(objref self)
{
    ClassObj *obj = (ClassObj *)self;
    char *name = obj->type->name;
    return string_new(name);
}

static objref class_str(objref self)
{
    ClassObj *obj = (ClassObj *)self;
    char *name = obj->type->name;
    char buf[64];
    sprintf(buf, "class %.32s", name);
    return string_new(buf);
}

void init_class_type(void)
{
    MethodDef class_methods[] = {
        /* clang-format off */
        METHOD_DEF("__name__",   nil, "s",          class_name),
        METHOD_DEF("__pkg__",    nil, "LClass;",    nil),
        METHOD_DEF("__mbrs__",   nil, "LArray(A);", nil),
        METHOD_DEF("get_field",  "s", "LField;",    nil),
        METHOD_DEF("get_method", "s", "LMethod;",   nil),
        METHOD_DEF("__str__",    nil, "s",          class_str),
        /* clang-format on */
    };

    type_add_methdefs(&class_type, class_methods);
    type_ready(&class_type);
    type_show(&class_type);
    pkg_add_type("/", &class_type);
}

#ifdef __cplusplus
}
#endif
