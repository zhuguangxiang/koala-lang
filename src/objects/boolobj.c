/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Boolean type definition                                                  |
 +---------------------------------------------------------------------------*/

static TValue bool_str(TValue *self, TValue *args, int nargs)
{
    Object *s = kl_new_fmt_str("%s", self->bval ? "true" : "false");
    return obj_value(s);
}

static MethodDef bool_methods[] = {
    { "__str__", bool_str },
    { NULL },
};

TypeObject bool_type = {
    ._type = &type_type,
    .name = "bool",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = bool_methods,
};

#ifdef __cplusplus
}
#endif
