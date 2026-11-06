/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * ByteOrder: the single class behind the package-level `BigEndian` and
 * `LittleEndian` values. Every method branches on the instance's endian
 * field, so BE/LE share one implementation.
 */

#include "binary_shim.h"
#include "bytebuf.h"

/* Field: endian(int) — declaration order in byte_order.kl. */
typedef struct _ByteOrderObject {
    OBJECT_HEAD
    TValue endian;
} ByteOrderObject;

static TypeObject byteorder_type;

#define BYTEORDER(self) ((ByteOrderObject *)SELF_AS(byteorder_type))

/* ====================================================================
 * Methods
 * ==================================================================== */

// __init__(endian int)
static TValue _bo_init(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    int64_t endian = to_int(args + 0);
    if (endian != ENDIAN_BIG && endian != ENDIAN_LITTLE) {
        raise_exc_str("binary: invalid endian");
        return error_value;
    }
    bo->endian = args[0];
    return no_value;
}

// read_u16(data bytes, offset int) uint16
static TValue _bo_read_u16(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    uint8_t *p = _bytes_ptr(args + 0, to_int(args + 1), 2);
    if (!p) return error_value;
    return uint16_value(_load_u16(p, _is_little(&bo->endian)));
}

// read_u32(data bytes, offset int) uint32
static TValue _bo_read_u32(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    uint8_t *p = _bytes_ptr(args + 0, to_int(args + 1), 4);
    if (!p) return error_value;
    return uint32_value(_load_u32(p, _is_little(&bo->endian)));
}

// read_u64(data bytes, offset int) uint64
static TValue _bo_read_u64(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    uint8_t *p = _bytes_ptr(args + 0, to_int(args + 1), 8);
    if (!p) return error_value;
    return uint64_value(_load_u64(p, _is_little(&bo->endian)));
}

// read_i16(data bytes, offset int) int16
static TValue _bo_read_i16(TValue *self, TValue *args, int nargs)
{
    TValue v = _bo_read_u16(self, args, nargs);
    if (is_error(&v)) return v;
    return int16_value((int16_t)(uint16_t)v.ival);
}

// read_i32(data bytes, offset int) int32
static TValue _bo_read_i32(TValue *self, TValue *args, int nargs)
{
    TValue v = _bo_read_u32(self, args, nargs);
    if (is_error(&v)) return v;
    return int32_value((int32_t)(uint32_t)v.ival);
}

// read_i64(data bytes, offset int) int64
static TValue _bo_read_i64(TValue *self, TValue *args, int nargs)
{
    TValue v = _bo_read_u64(self, args, nargs);
    if (is_error(&v)) return v;
    return int64_value((int64_t)(uint64_t)v.ival);
}

// read_f32(data bytes, offset int) float
static TValue _bo_read_f32(TValue *self, TValue *args, int nargs)
{
    TValue v = _bo_read_u32(self, args, nargs);
    if (is_error(&v)) return v;
    return float32_value(_bits_to_f32((uint32_t)v.ival));
}

// read_f64(data bytes, offset int) float
static TValue _bo_read_f64(TValue *self, TValue *args, int nargs)
{
    TValue v = _bo_read_u64(self, args, nargs);
    if (is_error(&v)) return v;
    return float64_value(_bits_to_f64((uint64_t)v.ival));
}

// write_u16(buf Buffer, v uint16)
static TValue _bo_write_u16(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    _store_u16(buf, to_uint16(args + 1), _is_little(&bo->endian));
    return no_value;
}

// write_u32(buf Buffer, v uint32)
static TValue _bo_write_u32(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    _store_u32(buf, to_uint32(args + 1), _is_little(&bo->endian));
    return no_value;
}

// write_u64(buf Buffer, v uint64)
static TValue _bo_write_u64(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    _store_u64(buf, to_uint64(args + 1), _is_little(&bo->endian));
    return no_value;
}

// write_i16(buf Buffer, v int16)
static TValue _bo_write_i16(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    _store_u16(buf, (uint16_t)to_int16(args + 1), _is_little(&bo->endian));
    return no_value;
}

// write_i32(buf Buffer, v int32)
static TValue _bo_write_i32(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    _store_u32(buf, (uint32_t)to_int32(args + 1), _is_little(&bo->endian));
    return no_value;
}

// write_i64(buf Buffer, v int64)
static TValue _bo_write_i64(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    _store_u64(buf, (uint64_t)to_int64(args + 1), _is_little(&bo->endian));
    return no_value;
}

// write_f32(buf Buffer, v float)
static TValue _bo_write_f32(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    uint32_t bits = _f32_to_bits((float)to_float(args + 1));
    _store_u32(buf, bits, _is_little(&bo->endian));
    return no_value;
}

// write_f64(buf Buffer, v float)
static TValue _bo_write_f64(TValue *self, TValue *args, int nargs)
{
    ByteOrderObject *bo = BYTEORDER(self);
    Object *bb = to_obj(args + 0);
    Buffer *buf = &BYTEBUF_BUF(bb);
    uint64_t bits = _f64_to_bits(to_float(args + 1));
    _store_u64(buf, bits, _is_little(&bo->endian));
    return no_value;
}

static MethodDef byteorder_methods[] = {
    { "__init__", _bo_init },       { "read_u16", _bo_read_u16 },   { "read_u32", _bo_read_u32 },
    { "read_u64", _bo_read_u64 },   { "read_i16", _bo_read_i16 },   { "read_i32", _bo_read_i32 },
    { "read_i64", _bo_read_i64 },   { "read_f32", _bo_read_f32 },   { "read_f64", _bo_read_f64 },
    { "write_u16", _bo_write_u16 }, { "write_u32", _bo_write_u32 }, { "write_u64", _bo_write_u64 },
    { "write_i16", _bo_write_i16 }, { "write_i32", _bo_write_i32 }, { "write_i64", _bo_write_i64 },
    { "write_f32", _bo_write_f32 }, { "write_f64", _bo_write_f64 }, { NULL, NULL },
};

static TypeObject byteorder_type = {
    ._type = &type_type,
    ._nfields = 1,
    .name = "ByteOrder",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(ByteOrderObject),
    .methdefs = byteorder_methods,
};

void binary_reg_byteorder(NativeLib *lib) { kl_reg_type(lib, &byteorder_type); }
