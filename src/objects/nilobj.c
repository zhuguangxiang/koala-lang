/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _nil_str(TValue *self, TValue *args, int nargs)
{
    Object *s = kl_new_str("nil");
    return obj_value(s);
}

static MethodDef _nil_methods[] = {
    { "__str__", _nil_str },
    { NULL },
};

TypeObject nil_type = {
    ._type = &type_type,
    .name = "NilType",
    .flags = TP_FLAGS_VALUE,
    .methdefs = _nil_methods,
};

#ifdef __cplusplus
}
#endif
