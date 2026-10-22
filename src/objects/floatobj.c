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

    return no_value;
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

/* Float hash: normalize -0.0 to +0.0 before hashing so numerically-equal
 * zeros collide on the same bucket, matching the eq fast-path
 * (`a->fval == b->fval`, IEEE numeric comparison). Without this the
 * equal-values-same-hash contract is broken and containers such as
 * dict[float, V] / hashset[float] would keep both zeros as distinct
 * elements. Aligns with Python / JavaScript / Go / C# semantics. NaN
 * passes through unchanged: NaN != NaN under the eq fast-path, so each
 * NaN insertion is a fresh element and hash stability is irrelevant. */
static TValue _float_hash(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    double v = to_raw_float(self);
    if (v == 0.0) v = 0.0;
    unsigned int hash = mem_hash(&v, sizeof(v));
    return int64_value(hash);
}

/* Float equality via IEEE numeric comparison, NOT the memcmp default.
 * Rationale: _default___eq__ compares TValue bytes, so -0.0 vs +0.0
 * would report unequal even though `a == b` at the VM OP_EQ fast-path
 * and inside container eq fast-paths reports equal — an inconsistency
 * that leaks whenever a caller reaches SLOT_EQ directly (e.g. via
 * kl_call_eq on an Equatable[T] generic instantiation). NaN semantics
 * fall out of IEEE `==` / `!=` for free: NaN != NaN, so __eq__ returns
 * false and __ne__ returns true. */
static TValue _float_eq(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double a = to_raw_float(self);
    double b = to_raw_float(&args[0]);
    return bool_value(a == b);
}

static TValue _float_ne(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    double a = to_raw_float(self);
    double b = to_raw_float(&args[0]);
    return bool_value(a != b);
}

static MethodDef float_methods[] = {
    { "fmt", _float_fmt },
    { "to_str", _float_to_str },
    { "hash", _float_hash },
    { "__eq__", _float_eq },
    { "__ne__", _float_ne },
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
