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

    if (is_float16(self)) {
        // float16: 3-4 significant digits, max ~5 chars for normal values
        snprintf(buf, 31, "%.5g", (double)(_Float16)self->fval);
    } else if (is_float32(self)) {
        // float32: 7-9 significant digits, use %.9g to guarantee round-trip
        snprintf(buf, 31, "%.9g", (double)(float)self->fval);
    } else if (is_float64(self)) {
        // float64: 15-17 significant digits, %.17g is correct
        snprintf(buf, 31, "%.17g", self->fval);
    }

    buf[31] = '\0';
    Object *sobj = kl_new_nstr(buf, strlen(buf));
    return obj_value(sobj);
}

static MethodDef float_methods[] = {
    { "__str__", _float_str },
    { NULL },
};

TypeObject float64_type = {
    ._type = &type_type,
    .name = "float64",
    .flags = TP_FLAGS_CLASS,
    .methdefs = float_methods,
};

TypeObject float32_type = {
    ._type = &type_type,
    .name = "float32",
    .flags = TP_FLAGS_CLASS,
    .methdefs = float_methods,
};

TypeObject float16_type = {
    ._type = &type_type,
    .name = "float16",
    .flags = TP_FLAGS_CLASS,
    .methdefs = float_methods,
};

#ifdef __cplusplus
}
#endif
