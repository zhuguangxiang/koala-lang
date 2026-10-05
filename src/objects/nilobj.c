/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _nil_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Formatter *fmt = kl_arg_fmt(0);
    kl_fmt_write_str(fmt, "nil", 3);
    return nil_value;
}

static MethodDef _nil_methods[] = {
    { "fmt", _nil_fmt },
    { NULL },
};

TypeObject nil_type = {
    ._type = &type_type,
    .name = "NilType",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_NIL,
    .methdefs = _nil_methods,
};

#ifdef __cplusplus
}
#endif
