/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _float_str(TValue *self, TValue *args, int nargs)
{
    char buf[32];
    snprintf(buf, 31, "%.17g", self->fval);
    buf[31] = '\0';
    Object *sobj = kl_new_nstr(buf, strlen(buf));
    return obj_value(sobj);
}

static MethodDef float_methods[] = {
    { "__str__", _float_str },
    { NULL },
};

TypeObject float_type = {
    ._type = &type_type,
    .name = "float64",
    .flags = TP_FLAGS_CLASS,
    .methdefs = float_methods,
};

#ifdef __cplusplus
}
#endif
