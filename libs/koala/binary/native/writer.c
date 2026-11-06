/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * BinWriter: stateful writer appending to an internal Buffer.
 */

#include "binary_shim.h"

/* Field: endian(int) — declaration order. Native state: Buffer. */
typedef struct _BinWriterObject {
    OBJECT_HEAD
    TValue endian;
    Buffer buf;
} BinWriterObject;

static TypeObject binwriter_type;

#define WRITER(self) ((BinWriterObject *)SELF_AS(binwriter_type))

static void _writer_free(Object *self)
{
    BinWriterObject *w = (BinWriterObject *)self;
    FINI_BUF(w->buf);
}

// __init__(endian int)
static TValue _writer_init(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    int64_t endian = to_int(args + 0);
    if (endian != ENDIAN_BIG && endian != ENDIAN_LITTLE) {
        raise_exc_str("binary: invalid endian");
        return error_value;
    }
    w->endian = args[0];
    INIT_BUF(w->buf);
    return no_value;
}

// len() int
static TValue _writer_len(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    return int64_value(BUF_LEN(w->buf));
}

// write_u8(v uint8)
static TValue _writer_write_u8(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _buf_push(&w->buf, (uint8_t)to_uint(args + 0));
    return no_value;
}

// write_u16(v uint16)
static TValue _writer_write_u16(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u16(&w->buf, (uint16_t)to_uint(args + 0), _is_little(&w->endian));
    return no_value;
}

// write_u32(v uint32)
static TValue _writer_write_u32(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u32(&w->buf, (uint32_t)to_uint(args + 0), _is_little(&w->endian));
    return no_value;
}

// write_u64(v uint64)
static TValue _writer_write_u64(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u64(&w->buf, (uint64_t)to_uint(args + 0), _is_little(&w->endian));
    return no_value;
}

// write_i8(v int8)
static TValue _writer_write_i8(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _buf_push(&w->buf, (uint8_t)to_int(args + 0));
    return no_value;
}

// write_i16(v int16)
static TValue _writer_write_i16(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u16(&w->buf, (uint16_t)to_int(args + 0), _is_little(&w->endian));
    return no_value;
}

// write_i32(v int32)
static TValue _writer_write_i32(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u32(&w->buf, (uint32_t)to_int(args + 0), _is_little(&w->endian));
    return no_value;
}

// write_i64(v int64)
static TValue _writer_write_i64(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u64(&w->buf, (uint64_t)to_int(args + 0), _is_little(&w->endian));
    return no_value;
}

// write_f32(v float)
static TValue _writer_write_f32(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u32(&w->buf, _f32_to_bits((float)to_float(args + 0)), _is_little(&w->endian));
    return no_value;
}

// write_f64(v float)
static TValue _writer_write_f64(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    _store_u64(&w->buf, _f64_to_bits(to_float(args + 0)), _is_little(&w->endian));
    return no_value;
}

// write_bytes(data bytes)
static TValue _writer_write_bytes(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    Object *bs = to_obj(args + 0);
    uint32_t n = BYTES_SIZE(bs);
    if (n > 0) {
        uint8_t *src = BYTES_DATA(bs) + BYTES_OFFSET(bs);
        buf_reserve(&w->buf, n);
        memcpy(w->buf.buf + w->buf.len, src, n);
        w->buf.len += n;
    }
    return no_value;
}

// write_uvarint(v uint64)
static TValue _writer_write_uvarint(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    uint64_t v = (uint64_t)to_uint(args + 0);
    while (v >= 0x80) {
        _buf_push(&w->buf, (uint8_t)v | 0x80);
        v >>= 7;
    }
    _buf_push(&w->buf, (uint8_t)v);
    return no_value;
}

// write_varint(v int64)
static TValue _writer_write_varint(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    int64_t sv = to_int(args + 0);
    // zigzag: shift as uint64, a signed `sv << 1` would be UB on overflow
    uint64_t v = ((uint64_t)sv << 1) ^ (uint64_t)(sv >> 63);
    while (v >= 0x80) {
        _buf_push(&w->buf, (uint8_t)v | 0x80);
        v >>= 7;
    }
    _buf_push(&w->buf, (uint8_t)v);
    return no_value;
}

// to_bytes() bytes
static TValue _writer_to_bytes(TValue *self, TValue *args, int nargs)
{
    BinWriterObject *w = WRITER(self);
    uint32_t n = (uint32_t)BUF_LEN(w->buf);
    Object *result = kl_new_bytes(n);
    if (n > 0) {
        memcpy(BYTES_DATA(result), w->buf.buf, n);
    }
    return obj_value(result);
}

static MethodDef binwriter_methods[] = {
    { "__init__", _writer_init },
    { "len", _writer_len },
    { "write_u8", _writer_write_u8 },
    { "write_u16", _writer_write_u16 },
    { "write_u32", _writer_write_u32 },
    { "write_u64", _writer_write_u64 },
    { "write_i8", _writer_write_i8 },
    { "write_i16", _writer_write_i16 },
    { "write_i32", _writer_write_i32 },
    { "write_i64", _writer_write_i64 },
    { "write_f32", _writer_write_f32 },
    { "write_f64", _writer_write_f64 },
    { "write_bytes", _writer_write_bytes },
    { "write_uvarint", _writer_write_uvarint },
    { "write_varint", _writer_write_varint },
    { "to_bytes", _writer_to_bytes },
    { NULL, NULL },
};

static TypeObject binwriter_type = {
    ._type = &type_type,
    ._nfields = 1,
    .name = "BinWriter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(BinWriterObject),
    .methdefs = binwriter_methods,
    ._free = _writer_free,
};

void binary_reg_writer(NativeLib *lib) { kl_reg_type(lib, &binwriter_type); }
