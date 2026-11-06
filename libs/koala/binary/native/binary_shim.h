/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Shared helpers for the koala/binary native module.
 *
 * One C file per Koala class:
 *   - byteorder.c: ByteOrder (behind BigEndian / LittleEndian globals)
 *   - reader.c:    BinReader
 *   - writer.c:    BinWriter
 *   - binary_shim.c: module init, registers all of the above
 */

#ifndef KOALA_BINARY_SHIM_H
#define KOALA_BINARY_SHIM_H

#include <string.h>
#include "bytesobj.h"
#include "excobj.h"

/* Endian ordinals (must match __binary__.kl constants). */
#define ENDIAN_BIG    0
#define ENDIAN_LITTLE 1

static inline int _is_little(TValue *endian) { return endian->ival == ENDIAN_LITTLE; }

/* Bounds-checked pointer into a Bytes object. Panics on OOB.
 * The check is written as `offset > size - n` so that a huge `offset`
 * cannot overflow `offset + n` into a negative value and slip through. */
static inline uint8_t *_bytes_ptr(TValue *data_val, int64_t offset, int64_t n)
{
    Object *obj = to_obj(data_val);
    uint8_t *base = BYTES_DATA(obj) + BYTES_OFFSET(obj);
    int64_t size = (int64_t)BYTES_SIZE(obj);

    if (offset < 0 || n < 0 || offset > size - n) {
        raise_exc_str("binary: read out of bounds");
        return NULL;
    }
    return base + offset;
}

/* Push a byte into a C Buffer (auto-grows). */
static inline void _buf_push(Buffer *buf, uint8_t v) { buf_write_byte(buf, v); }

/* Endian-aware load/store helpers, shared by ByteOrder, BinReader and BinWriter.
 * `little` is 1 for little-endian, 0 for big-endian. */
static inline uint16_t _load_u16(uint8_t *p, int little)
{
    if (little) {
        return (uint16_t)(((uint16_t)p[1] << 8) | (uint16_t)p[0]);
    }
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static inline uint32_t _load_u32(uint8_t *p, int little)
{
    if (little) {
        return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) |
               (uint32_t)p[0];
    }
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline uint64_t _load_u64(uint8_t *p, int little)
{
    uint64_t v = 0;
    if (little) {
        for (int i = 7; i >= 0; i--) v = (v << 8) | (uint64_t)p[i];
    } else {
        for (int i = 0; i < 8; i++) v = (v << 8) | (uint64_t)p[i];
    }
    return v;
}

static inline void _store_u16(Buffer *buf, uint16_t v, int little)
{
    if (little) {
        _buf_push(buf, (uint8_t)v);
        _buf_push(buf, (uint8_t)(v >> 8));
    } else {
        _buf_push(buf, (uint8_t)(v >> 8));
        _buf_push(buf, (uint8_t)v);
    }
}

static inline void _store_u32(Buffer *buf, uint32_t v, int little)
{
    if (little) {
        _buf_push(buf, (uint8_t)v);
        _buf_push(buf, (uint8_t)(v >> 8));
        _buf_push(buf, (uint8_t)(v >> 16));
        _buf_push(buf, (uint8_t)(v >> 24));
    } else {
        _buf_push(buf, (uint8_t)(v >> 24));
        _buf_push(buf, (uint8_t)(v >> 16));
        _buf_push(buf, (uint8_t)(v >> 8));
        _buf_push(buf, (uint8_t)v);
    }
}

static inline void _store_u64(Buffer *buf, uint64_t v, int little)
{
    if (little) {
        for (int i = 0; i < 64; i += 8) _buf_push(buf, (uint8_t)(v >> i));
    } else {
        for (int i = 56; i >= 0; i -= 8) _buf_push(buf, (uint8_t)(v >> i));
    }
}

/* Float bit-cast helpers (type-pun via memcpy, no UB). */
static inline float _bits_to_f32(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

static inline uint32_t _f32_to_bits(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

static inline double _bits_to_f64(uint64_t bits)
{
    double d;
    memcpy(&d, &bits, sizeof(d));
    return d;
}

static inline uint64_t _f64_to_bits(double d)
{
    uint64_t bits;
    memcpy(&bits, &d, sizeof(bits));
    return bits;
}

/* Per-class registration, implemented in byteorder.c / reader.c / writer.c. */
void binary_reg_byteorder(NativeLib *lib);
void binary_reg_reader(NativeLib *lib);
void binary_reg_writer(NativeLib *lib);

#endif /* KOALA_BINARY_SHIM_H */
