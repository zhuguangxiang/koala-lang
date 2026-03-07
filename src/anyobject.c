/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static unsigned int any_hash(Value *self)
{
    unsigned int v = mem_hash(self, sizeof(Value));
    return v;
}

static int any_compare(Value *self, Value *rhs, int op)
{
    int r = memcmp(self, rhs, sizeof(Value));
    return r;
}

static Value any_str(Value *self)
{
    TypeObject *tp = object_typeof(self);
    Object *res = kl_new_fmt_str("<%s object>", tp->name);
    return obj_value(res);
}

// static Value any_equal(Value *self, Value *rhs)
// {
//     int r = !memcmp(self, rhs, sizeof(Value));
//     return int64_value(r);
// }

// static MethodDef any_methods[] = {
//     { "__hash__", any_hash, METH_NO_ARGS },
//     { "__eq__", any_equal, METH_ONE_ARG },
//     { "__str__", any_str, METH_NO_ARGS },
//     { NULL },
// };

// clang-format off
TypeObject any_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "any",
    .flags = TP_FLAGS_TRAIT,
    .hash = any_hash,
    .cmp = any_compare,
    .str = any_str,
    // .methdefs = any_methods,
};
// clang-format on

#ifdef __cplusplus
}
#endif
