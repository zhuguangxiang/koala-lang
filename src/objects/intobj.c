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

    if (is_intf(self)) {
        IntfTable *itab = (IntfTable *)(self)->itab;
        ASSERT(itab);
        TypeObject *tp = itab->tp;
        tag = tp->tag;
    }

    if (is_int_tag(tag)) {
        return snprintf(buf, size, "%" PRId64, (int64_t)self->ival);
    }

    if (is_uint_tag(tag)) {
        return snprintf(buf, size, "%" PRIu64, (uint64_t)self->ival);
    }

    UNREACHABLE();
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

//
// Operators
//
// Statically typed code runs them as intrinsic instructions (OP_INT_*, OP_UINT_*). These natives
// are what a trait value such as `Arithmetic[int]` calls through its interface table, so they
// follow the same semantics: 64-bit wrapping arithmetic, and `self` may be an interface value.
//

static int _is_unsigned(TValue *self)
{
    uint64_t tag = self->tag;

    if (is_intf(self)) {
        IntfTable *itab = (IntfTable *)(self)->itab;
        ASSERT(itab);
        TypeObject *tp = itab->tp;
        tag = tp->tag;
    }

    return is_uint_tag(tag);
}

/* the result has the signedness of `self` */
static TValue _int_result(TValue *self, uint64_t bits)
{
    if (_is_unsigned(self)) return uint64_value(bits);
    return int64_value((int64_t)bits);
}

static TValue _int_add(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs + rhs);
}

static TValue _int_sub(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs - rhs);
}

static TValue _int_mul(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs * rhs);
}

static TValue _int_div(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (rhs == 0) {
        raise_exc_str("division by zero");
        return error_value;
    }

    if (_is_unsigned(self)) return uint64_value(lhs / rhs);

    // INT64_MIN / -1 overflows and traps in C: it wraps to INT64_MIN, like '-' does
    if ((int64_t)rhs == -1) return int64_value((int64_t)(0 - lhs));

    return int64_value((int64_t)lhs / (int64_t)rhs);
}

static TValue _int_mod(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (rhs == 0) {
        raise_exc_str("division by zero");
        return error_value;
    }

    if (_is_unsigned(self)) return uint64_value(lhs % rhs);

    // INT64_MIN % -1 traps in C too
    if ((int64_t)rhs == -1) return int64_value(0);

    return int64_value((int64_t)lhs % (int64_t)rhs);
}

static TValue _int_neg(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t val = _get_raw_bits(self);
    return _int_result(self, 0 - val);
}

static TValue _int_shl(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    if (!kl_check_shift_count(&args[0])) return error_value;

    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs << rhs);
}

static TValue _int_shr(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    if (!kl_check_shift_count(&args[0])) return error_value;

    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (_is_unsigned(self)) return uint64_value(lhs >> rhs);
    return int64_value((int64_t)lhs >> rhs);
}

static TValue _int_bitand(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs & rhs);
}

static TValue _int_bitor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs | rhs);
}

static TValue _int_bitxor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return _int_result(self, lhs ^ rhs);
}

static TValue _int_bitnot(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    uint64_t val = _get_raw_bits(self);
    return _int_result(self, ~val);
}

static TValue _int_lt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (_is_unsigned(self)) return bool_value(lhs < rhs);
    return bool_value((int64_t)lhs < (int64_t)rhs);
}

static TValue _int_le(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (_is_unsigned(self)) return bool_value(lhs <= rhs);
    return bool_value((int64_t)lhs <= (int64_t)rhs);
}

static TValue _int_gt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (_is_unsigned(self)) return bool_value(lhs > rhs);
    return bool_value((int64_t)lhs > (int64_t)rhs);
}

static TValue _int_ge(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);

    if (_is_unsigned(self)) return bool_value(lhs >= rhs);
    return bool_value((int64_t)lhs >= (int64_t)rhs);
}

static TValue _int_eq(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return bool_value(lhs == rhs);
}

static TValue _int_ne(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    uint64_t lhs = _get_raw_bits(self);
    uint64_t rhs = _get_raw_bits(&args[0]);
    return bool_value(lhs != rhs);
}

static MethodDef int_methods[] = {
    { "__add__", _int_add },
    { "__sub__", _int_sub },
    { "__mul__", _int_mul },
    { "__div__", _int_div },
    { "__mod__", _int_mod },
    { "__neg__", _int_neg },
    { "__shl__", _int_shl },
    { "__shr__", _int_shr },
    { "__bitand__", _int_bitand },
    { "__bitor__", _int_bitor },
    { "__bitxor__", _int_bitxor },
    { "__bitnot__", _int_bitnot },
    { "__lt__", _int_lt },
    { "__le__", _int_le },
    { "__gt__", _int_gt },
    { "__ge__", _int_ge },
    { "__eq__", _int_eq },
    { "__ne__", _int_ne },
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
