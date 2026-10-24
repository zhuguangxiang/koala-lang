/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bytesobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _int_to_buf(char *buf, size_t size, TValue *self)
{
    uint64_t tag = self->tag;
    if (is_int_tag(tag)) {
        return snprintf(buf, size, "%" PRId64, (int64_t)self->ival);
    } else if (is_uint_tag(tag)) {
        return snprintf(buf, size, "%" PRIu64, (uint64_t)self->ival);
    } else {
        UNREACHABLE();
        return 0;
    }
}

static uint64_t _get_raw_bits(TValue *self)
{
    if (is_int(self) || is_uint(self)) {
        return (uint64_t)self->ival;
    }

    ASSERT(is_intf(self));
    IntfTable *itab = (IntfTable *)(self)->itab;
    ASSERT(itab);
    TypeObject *tp = itab->tp;
    ASSERT(is_int_tag(tp->tag) || is_uint_tag(tp->tag));

    return (uint64_t)self->ival;
}

static int64_t _get_raw_int(TValue *self)
{
    if (is_int(self)) {
        return (int64_t)self->ival;
    } else if (is_uint(self)) {
        return (int64_t)self->ival;
    }

    ASSERT(is_intf(self));
    IntfTable *itab = (IntfTable *)(self)->itab;
    ASSERT(itab);
    TypeObject *tp = itab->tp;
    if (is_int_tag(tp->tag)) {
        return (int64_t)self->ival;
    } else if (is_uint_tag(tp->tag)) {
        return (int64_t)self->ival;
    }

    UNREACHABLE();
    return 0;
}

static TValue _int_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char buf[64];
    int len = _int_to_buf(buf, sizeof(buf), self);
    ASSERT(len >= 0 && len < (int)sizeof(buf));
    Formatter *fmt = kl_arg_obj_as(0, fmt_type);
    buf_write_nstr(&fmt->buf, buf, len);

    return no_value;
}

static TValue _int_to_str(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    char buf[64];
    int len = _int_to_buf(buf, sizeof(buf), self);
    ASSERT(len >= 0 && len < (int)sizeof(buf));
    Object *sobj = kl_new_nstr(buf, len);
    return obj_value(sobj);
}

//
// Bit manipulation methods
//

static TValue _int_bit_length(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    int64_t val = _get_raw_int(self);
    if (val == 0) return int64_value(0);
    if (val < 0) val = -val;
    return int64_value(64 - __builtin_clzll((uint64_t)val));
}

static TValue _int_bit_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t val = _get_raw_bits(self);
    return int64_value(__builtin_popcountll(val));
}

static TValue _int_ctz(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t val = _get_raw_bits(self);
    if (val == 0) return int64_value(64);
    return int64_value(__builtin_ctzll(val));
}

static TValue _int_clz(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t val = _get_raw_bits(self);
    if (val == 0) return int64_value(64);
    return int64_value(__builtin_clzll(val));
}

static TValue _int_ffs(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t val = _get_raw_bits(self);
    if (val == 0) return int64_value(0);
    return int64_value(__builtin_ffsll(val));
}

static TValue _int_bit_reverse(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t v = _get_raw_bits(self);

    // Standard bit reversal algorithm
    v = ((v >> 1) & 0x5555555555555555ULL) | ((v & 0x5555555555555555ULL) << 1);
    v = ((v >> 2) & 0x3333333333333333ULL) | ((v & 0x3333333333333333ULL) << 2);
    v = ((v >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((v & 0x0F0F0F0F0F0F0F0FULL) << 4);
    v = ((v >> 8) & 0x00FF00FF00FF00FFULL) | ((v & 0x00FF00FF00FF00FFULL) << 8);
    v = ((v >> 16) & 0x0000FFFF0000FFFFULL) | ((v & 0x0000FFFF0000FFFFULL) << 16);
    v = (v >> 32) | (v << 32);

    return int64_value((int64_t)v);
}

static TValue _int_to_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t length = kl_arg_int(0);
    int byteorder = (int)kl_arg_int(1);

    if (length < 0) {
        raise_exc_str("to_bytes: length must be non-negative");
        return error_value;
    }

    uint64_t val = _get_raw_bits(self);
    Object *bobj = kl_new_bytes((uint32_t)length);
    uint8_t *data = BYTES_DATA(bobj);

    if (byteorder == 0) { // BIG_ENDIAN
        for (int64_t i = length - 1; i >= 0; i--) {
            data[i] = (uint8_t)(val & 0xFF);
            val >>= 8;
        }
    } else { // LITTLE_ENDIAN
        for (int64_t i = 0; i < length; i++) {
            data[i] = (uint8_t)(val & 0xFF);
            val >>= 8;
        }
    }

    return obj_value(bobj);
}

static TValue _int_from_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Object *bobj = kl_arg_obj_as(0, bytes_type);
    int byteorder = (int)kl_arg_int(1);

    uint8_t *data = BYTES_DATA(bobj);
    uint32_t size = BYTES_SIZE(bobj);
    uint64_t val = 0;

    if (byteorder == 0) { // BIG_ENDIAN
        for (uint32_t i = 0; i < size; i++) {
            val = (val << 8) | data[i];
        }
    } else { // LITTLE_ENDIAN
        for (uint32_t i = size; i > 0; i--) {
            val = (val << 8) | data[i - 1];
        }
    }

    return int64_value((int64_t)val);
}

static MethodDef int_methods[] = {
    { "fmt", _int_fmt },
    { "to_str", _int_to_str },
    { "bit_length", _int_bit_length },
    { "bit_count", _int_bit_count },
    { "bit_ctz", _int_ctz },
    { "bit_clz", _int_clz },
    { "bit_ffs", _int_ffs },
    { "bit_reverse", _int_bit_reverse },
    { "to_bytes", _int_to_bytes },
    { "from_bytes", _int_from_bytes },
    { NULL },
};

TypeObject int8_type = {
    ._type = &type_type,
    .name = "int8",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_INT8,
    .methdefs = int_methods,
};

TypeObject int16_type = {
    ._type = &type_type,
    .name = "int16",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_INT16,
    .methdefs = int_methods,
};

TypeObject int32_type = {
    ._type = &type_type,
    .name = "int32",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_INT32,
    .methdefs = int_methods,
};

TypeObject int64_type = {
    ._type = &type_type,
    .name = "int64",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_INT64,
    .methdefs = int_methods,
};

TypeObject uint8_type = {
    ._type = &type_type,
    .name = "uint8",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_UINT8,
    .methdefs = int_methods,
};

TypeObject uint16_type = {
    ._type = &type_type,
    .name = "uint16",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_UINT16,
    .methdefs = int_methods,
};

TypeObject uint32_type = {
    ._type = &type_type,
    .name = "uint32",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_UINT32,
    .methdefs = int_methods,
};

TypeObject uint64_type = {
    ._type = &type_type,
    .name = "uint64",
    .flags = TP_FLAGS_VALUE,
    .tag = TAG_UINT64,
    .methdefs = int_methods,
};

#ifdef __cplusplus
}
#endif
