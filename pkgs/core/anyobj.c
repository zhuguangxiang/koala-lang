/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"
#include "util/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

int32 tp_any_hash(anyref obj, int ref)
{
    if (!ref) return (int32)mem_hash(&obj, sizeof(anyref));

    /* __hash__ */
    VTable *vtbl = __GET_VTBL(obj);
    FuncNode *fn = vtbl->func[0];

    if (fn->kind == MNODE_CFUNC_KIND) {
        return ((int32(*)(objref))fn->ptr)(obj);
    }

    // not implemented yet
    return 0;
}

bool tp_any_equal(anyref obj, anyref other, int ref)
{
    if (obj == other) return 1;
    if (!ref) return 0;

    /* __eq__ */
    VTable *vtbl = __GET_VTBL(obj);
    FuncNode *fn = vtbl->func[1];

    if (fn->kind == MNODE_CFUNC_KIND) {
        return ((bool (*)(objref, objref))fn->ptr)(obj, other);
    }

    // not implemented yet
    return 0;
}

/*
trait Any {
    func __hash__() int32;
    func __eq__(other Any) bool;
    func __class__() Class;
    func __str__() string;
}
*/

TypeInfo any_type = {
    .name = "Any",
    .flags = TF_TRAIT,
};

static int32 any_hash(objref self)
{
    return (int32)mem_hash(&self, sizeof(objref));
}

static bool any_equal(objref self, objref other)
{
    TypeInfo *t1 = __GET_TYPE(self);
    TypeInfo *t2 = __GET_TYPE(other);

    if (t1 != t2) return 0;
    return self == other;
}

static objref any_class(objref self)
{
    return class_new(self);
}

static objref any_str(objref self)
{
    char buf[64];
    TypeInfo *type = __GET_TYPE(self);
    snprintf(buf, sizeof(buf) - 1, "%.32s@%lx", type->name, self);
    return string_new(buf);
}

void init_any_type(void)
{
    MethodDef any_methods[] = {
        /* clang-format off */
        /* DO NOT change the order */
        METHOD_DEF("__hash__",  nil, "i32",     any_hash),
        METHOD_DEF("__eq__",    "A", "b",       any_equal),
        METHOD_DEF("__class__", nil, "LClass;", any_class),
        METHOD_DEF("__str__",   nil, "s",       any_str),
        /* clang-format on */
    };

    type_add_methdefs(&any_type, any_methods);
    type_ready(&any_type);
    type_show(&any_type);
    pkg_add_type("/", &any_type);
}

#ifdef __cplusplus
}
#endif
