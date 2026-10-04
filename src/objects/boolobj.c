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

static TValue _bool_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int v = to_raw_bool(self);
    Formatter *fobj = kl_arg_obj_as(0, fmt_type);
    buf_write_str(&fobj->buf, v ? "true" : "false");
    return nil_value;
}

static TValue _bool_to_str(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    int v = to_raw_bool(self);
    Object *sobj = kl_new_nstr(v ? "true" : "false", v ? 4 : 5);
    return obj_value(sobj);
}

static MethodDef bool_methods[] = {
    { "fmt", _bool_fmt },
    { "to_str", _bool_to_str },
    { NULL },
};

TypeObject bool_type = {
    ._type = &type_type,
    .name = "bool",
    .flags = TP_FLAGS_VALUE,
    .methdefs = bool_methods,
};

#ifdef __cplusplus
}
#endif
