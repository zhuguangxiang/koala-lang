/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * BinReader: stateful reader with bound endianness, panics on OOB.
 */

#include "binary_shim.h"

/* Fields: data(bytes), endian(int), pos(int) — declaration order. */
typedef struct _BinReaderObject {
    OBJECT_HEAD
    TValue data;
    TValue endian;
    TValue pos;
} BinReaderObject;

static TypeObject binreader_type;

#define READER(self) ((BinReaderObject *)SELF_AS(binreader_type))

static inline uint8_t *_reader_ptr(BinReaderObject *r, int64_t n)
{
    return _bytes_ptr(&r->data, r->pos.ival, n);
}

// __init__(data bytes, endian int)
static TValue _reader_init(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    int64_t endian = to_int(args + 1);
    if (endian != ENDIAN_BIG && endian != ENDIAN_LITTLE) {
        raise_exc_str("binary: invalid endian");
        return error_value;
    }
    r->data = args[0];
    r->endian = args[1];
    r->pos = int64_value(0);
    return no_value;
}

// remaining() int
static TValue _reader_remaining(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    Object *data = to_obj(&r->data);
    int64_t size = (int64_t)BYTES_SIZE(data);
    return int64_value(size - r->pos.ival);
}

// is_empty() bool
static TValue _reader_is_empty(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    Object *data = to_obj(&r->data);
    int64_t size = (int64_t)BYTES_SIZE(data);
    return bool_value(r->pos.ival >= size);
}

// skip(n int)
static TValue _reader_skip(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    int64_t n = to_int(args + 0);
    Object *data = to_obj(&r->data);
    int64_t size = (int64_t)BYTES_SIZE(data);

    if (n < 0) {
        raise_exc_str("binary: negative skip");
        return error_value;
    }
    // pos <= size is an invariant of every reader operation, so size - pos
    // cannot underflow here.
    if (n > size - r->pos.ival) {
        raise_exc_str("binary: skip out of bounds");
        return error_value;
    }
    r->pos.ival += n;
    return no_value;
}

// read_u8() uint8
static TValue _reader_read_u8(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    uint8_t *p = _reader_ptr(r, 1);
    if (!p) return error_value;
    r->pos.ival += 1;
    return uint8_value(p[0]);
}

// read_u16() uint16
static TValue _reader_read_u16(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    uint8_t *p = _reader_ptr(r, 2);
    if (!p) return error_value;
    r->pos.ival += 2;
    return uint16_value(_load_u16(p, _is_little(&r->endian)));
}

// read_u32() uint32
static TValue _reader_read_u32(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    uint8_t *p = _reader_ptr(r, 4);
    if (!p) return error_value;
    r->pos.ival += 4;
    return uint32_value(_load_u32(p, _is_little(&r->endian)));
}

// read_u64() uint64
static TValue _reader_read_u64(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    uint8_t *p = _reader_ptr(r, 8);
    if (!p) return error_value;
    r->pos.ival += 8;
    return uint64_value(_load_u64(p, _is_little(&r->endian)));
}

// read_i8() int8
static TValue _reader_read_i8(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_u8(self, args, nargs);
    if (is_error(&v)) return v;
    return int8_value((int8_t)v.ival);
}

// read_i16() int16
static TValue _reader_read_i16(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_u16(self, args, nargs);
    if (is_error(&v)) return v;
    return int16_value((int16_t)(uint16_t)v.ival);
}

// read_i32() int32
static TValue _reader_read_i32(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_u32(self, args, nargs);
    if (is_error(&v)) return v;
    return int32_value((int32_t)(uint32_t)v.ival);
}

// read_i64() int64
static TValue _reader_read_i64(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_u64(self, args, nargs);
    if (is_error(&v)) return v;
    return int64_value((int64_t)(uint64_t)v.ival);
}

// read_f32() float
static TValue _reader_read_f32(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_u32(self, args, nargs);
    if (is_error(&v)) return v;
    return float32_value(_bits_to_f32((uint32_t)v.ival));
}

// read_f64() float
static TValue _reader_read_f64(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_u64(self, args, nargs);
    if (is_error(&v)) return v;
    return float64_value(_bits_to_f64((uint64_t)v.ival));
}

// read_bytes(n int) bytes
static TValue _reader_read_bytes(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    int64_t n = to_int(args + 0);
    uint8_t *p = _reader_ptr(r, n);
    if (!p) return error_value;
    r->pos.ival += n;
    Object *result = kl_new_bytes((uint32_t)n);
    memcpy(BYTES_DATA(result), p, (size_t)n);
    return obj_value(result);
}

// read_uvarint() uint64
static TValue _reader_read_uvarint(TValue *self, TValue *args, int nargs)
{
    BinReaderObject *r = READER(self);
    Object *data_obj = to_obj(&r->data);
    uint8_t *base = BYTES_DATA(data_obj) + BYTES_OFFSET(data_obj);
    int64_t size = (int64_t)BYTES_SIZE(data_obj);
    uint64_t result = 0;
    int shift = 0;

    while (r->pos.ival < size) {
        uint8_t b = base[r->pos.ival];
        r->pos.ival += 1;
        // A 64-bit LEB128 value spans at most 10 bytes; the 10th byte may
        // only carry the top bit. Anything longer (or wider) is malformed.
        if (shift > 63 || (shift == 63 && b > 1)) {
            raise_exc_str("binary: varint overflow");
            return error_value;
        }
        result |= (uint64_t)(b & 0x7F) << shift;
        if (b < 0x80) return uint64_value(result);
        shift += 7;
    }

    raise_exc_str("binary: incomplete uvarint");
    return error_value;
}

// read_varint() int64
static TValue _reader_read_varint(TValue *self, TValue *args, int nargs)
{
    TValue v = _reader_read_uvarint(self, args, nargs);
    if (is_error(&v)) return v;
    uint64_t uval = (uint64_t)v.ival;
    int64_t val = (int64_t)(uval >> 1) ^ -((int64_t)(uval & 1));
    return int64_value(val);
}

static MethodDef binreader_methods[] = {
    { "__init__", _reader_init },           { "remaining", _reader_remaining },
    { "is_empty", _reader_is_empty },       { "skip", _reader_skip },
    { "read_u8", _reader_read_u8 },         { "read_u16", _reader_read_u16 },
    { "read_u32", _reader_read_u32 },       { "read_u64", _reader_read_u64 },
    { "read_i8", _reader_read_i8 },         { "read_i16", _reader_read_i16 },
    { "read_i32", _reader_read_i32 },       { "read_i64", _reader_read_i64 },
    { "read_f32", _reader_read_f32 },       { "read_f64", _reader_read_f64 },
    { "read_bytes", _reader_read_bytes },   { "read_uvarint", _reader_read_uvarint },
    { "read_varint", _reader_read_varint }, { NULL, NULL },
};

static TypeObject binreader_type = {
    ._type = &type_type,
    ._nfields = 3,
    .name = "BinReader",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(BinReaderObject),
    .methdefs = binreader_methods,
};

void binary_reg_reader(NativeLib *lib) { kl_reg_type(lib, &binreader_type); }
