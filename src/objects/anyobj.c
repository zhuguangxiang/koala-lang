/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Any type definition                                                      |
 +---------------------------------------------------------------------------*/

static TValue any_hash(TValue *self, TValue *args, int nargs)
{
    unsigned int v = mem_hash(self, sizeof(TValue));
    return int64_value(v);
}

static TValue any_equals(TValue *self, TValue *args, int nargs)
{
    TValue *rhs = &args[0];
    int r = memcmp(self, rhs, sizeof(TValue));
    RETURN_RICHCOMPARE(r, CMP_EQ);
}

static TValue any_str(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    Object *res = kl_new_fmt_str("<%s object>", tp->name);
    return obj_value(res);
}

static MethodDef any_methods[] = {
    { "__hash__", any_hash },
    { "__eq__", any_equals },
    { "__str__", any_str },
    { NULL },
};

TypeObject any_type = {
    ._type = &type_type,
    .name = "any",
    .flags = TP_FLAGS_TRAIT | TP_FLAGS_PUBLIC,
    .methdefs = any_methods,
};

#ifdef __cplusplus
}
#endif
