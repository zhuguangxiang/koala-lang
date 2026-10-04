/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _int_fmt(TValue *self, TValue *args, int nargs)
{
    char buf[64];

    uint64_t tag = self->tag;
    if (is_int_tag(tag)) {
        snprintf(buf, sizeof(buf), "%" PRId64, (int64_t)self->ival);
    } else if (is_uint_tag(tag)) {
        snprintf(buf, sizeof(buf), "%" PRIu64, (uint64_t)self->ival);
    } else {
        UNREACHABLE();
    }

    buf[sizeof(buf) - 1] = '\0';

    Formatter *fmt = kl_arg_obj_as(0, fmt_type);
    buf_write_nstr(&fmt->buf, buf, strlen(buf));

    return nil_value;
}

static TValue _int_to_str(TValue *self, TValue *args, int nargs)
{
    char buf[64];
    int len = 0;

    uint64_t tag = self->tag;
    if (is_int_tag(tag)) {
        len = snprintf(buf, sizeof(buf), "%" PRId64, (int64_t)self->ival);
    } else if (is_uint_tag(tag)) {
        len = snprintf(buf, sizeof(buf), "%" PRIu64, (uint64_t)self->ival);
    } else {
        UNREACHABLE();
    }

    ASSERT(len >= 0);
    Object *sobj = kl_new_nstr(buf, len);
    return obj_value(sobj);
}

static MethodDef int_methods[] = {
    { "fmt", _int_fmt },
    { "to_str", _int_to_str },
    { NULL },
};

TypeObject int8_type = {
    ._type = &type_type,
    .name = "int8",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject int16_type = {
    ._type = &type_type,
    .name = "int16",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject int32_type = {
    ._type = &type_type,
    .name = "int32",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject int64_type = {
    ._type = &type_type,
    .name = "int64",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject uint8_type = {
    ._type = &type_type,
    .name = "uint8",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject uint16_type = {
    ._type = &type_type,
    .name = "uint16",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject uint32_type = {
    ._type = &type_type,
    .name = "uint32",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

TypeObject uint64_type = {
    ._type = &type_type,
    .name = "uint64",
    .flags = TP_FLAGS_VALUE,
    .methdefs = int_methods,
};

#ifdef __cplusplus
}
#endif
