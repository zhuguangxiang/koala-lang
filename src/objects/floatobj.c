/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLOAT_BUF_SIZE 64

static int _float_to_buf(char *buf, int tag, double fval)
{
    int len = 0;

    if (tag == TAG_FLOAT16) {
        // float16: about 3-4 decimal digits of precision; %.5g preserves
        // all float16 values without exposing excessive formatting noise.
        len = snprintf(buf, FLOAT_BUF_SIZE, "%.5g", (double)(_Float16)fval);
    } else if (tag == TAG_FLOAT32) {
        // float32: about 7 decimal digits of precision; %.7g avoids
        // exposing binary floating-point representation noise.
        len = snprintf(buf, FLOAT_BUF_SIZE, "%.7g", (double)(float)fval);
    } else if (tag == TAG_FLOAT64) {
        // float64: about 15-16 decimal digits of precision; %.15g avoids
        // exposing binary floating-point noise such as 3.1400000000000001.
        len = snprintf(buf, FLOAT_BUF_SIZE, "%.16g", fval);
    } else if (tag == TAG_BFLOAT16) {
        // bfloat16: about 2-3 decimal digits of precision; %.5g preserves
        // all bfloat16 values without exposing excessive formatting noise.
        len = snprintf(buf, FLOAT_BUF_SIZE, "%.5g", (double)(__bf16)fval);
    } else {
        UNREACHABLE();
    }

    return len;
}

static TValue _float_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    char buf[FLOAT_BUF_SIZE];

    int tag;
    if (is_intf(self)) {
        IntfTable *itab = (IntfTable *)(self)->itab;
        ASSERT(itab);
        TypeObject *tp = itab->tp;
        tag = tp->tag;
        ASSERT(is_float_tag(tag));
    } else {
        tag = self->tag;
    }

    int len = _float_to_buf(buf, tag, self->fval);
    ASSERT(len >= 0 && len < (int)sizeof(buf));

    Formatter *fmt = kl_arg_obj_as(0, fmt_type);
    buf_write_nstr(&fmt->buf, buf, len);

    return nil_value;
}

static TValue _float_to_str(TValue *self, TValue *args, int nargs)
{
    char buf[FLOAT_BUF_SIZE];

    int tag;
    if (is_intf(self)) {
        IntfTable *itab = (IntfTable *)(self)->itab;
        ASSERT(itab);
        TypeObject *tp = itab->tp;
        tag = tp->tag;
        ASSERT(is_float_tag(tag));
    } else {
        tag = self->tag;
    }

    int len = _float_to_buf(buf, tag, self->fval);
    ASSERT(len >= 0 && len < (int)sizeof(buf));

    Object *sobj = kl_new_nstr(buf, len);
    return obj_value(sobj);
}

static MethodDef float_methods[] = {
    { "fmt", _float_fmt },
    { "to_str", _float_to_str },
    { NULL },
};

TypeObject float64_type = {
    ._type = &type_type,
    .name = "float64",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_FLOAT64,
    .methdefs = float_methods,
};

TypeObject float32_type = {
    ._type = &type_type,
    .name = "float32",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_FLOAT32,
    .methdefs = float_methods,
};

TypeObject float16_type = {
    ._type = &type_type,
    .name = "float16",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_FLOAT16,
    .methdefs = float_methods,
};

TypeObject bfloat16_type = {
    ._type = &type_type,
    .name = "bfloat16",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_BFLOAT16,
    .methdefs = float_methods,
};

#ifdef __cplusplus
}
#endif
