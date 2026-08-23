/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _none_str(TValue *self, TValue *args, int nargs)
{
    Object *s = kl_new_str("none");
    return obj_value(s);
}

static MethodDef _none_methods[] = {
    { "__str__", _none_str },
    { NULL },
};

TypeObject none_type = {
    ._type = &type_type,
    .name = "NoneType",
    .flags = TP_FLAGS_VALUE,
    .methdefs = _none_methods,
};

#ifdef __cplusplus
}
#endif
