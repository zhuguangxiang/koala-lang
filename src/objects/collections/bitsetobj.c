/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/bitsetobj.h"
#include "buffer.h"
#include "bytesobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

// __init__(size int)
static TValue _bitset_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs = SELF_AS(bitset_type);
    int64_t size = kl_arg_int64(0);

    if (size < 0) {
        raise_exc_str("BitSet size must be non-negative");
        return error_value;
    }

    init_bitset(&bs->bitset, (int)size);
    return no_value;
}

// set(bit int)
static TValue _bitset_set(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs = SELF_AS(bitset_type);
    int64_t bit = kl_arg_int64(0);

    if (bit < 0 || bit >= BIT_SIZE(&bs->bitset)) {
        raise_exc_fmt("bit index %lld out of range [0, %d)", bit, BIT_SIZE(&bs->bitset));
        return error_value;
    }

    bitset_set(&bs->bitset, (int)bit);
    return no_value;
}

// clear(bit int)
static TValue _bitset_clear(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs = SELF_AS(bitset_type);
    int64_t bit = kl_arg_int64(0);

    if (bit < 0 || bit >= BIT_SIZE(&bs->bitset)) {
        raise_exc_fmt("bit index %lld out of range [0, %d)", bit, BIT_SIZE(&bs->bitset));
        return error_value;
    }

    bitset_clear(&bs->bitset, (int)bit);
    return no_value;
}

// test(bit int) -> bool
static TValue _bitset_test(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs = SELF_AS(bitset_type);
    int64_t bit = kl_arg_int64(0);

    if (bit < 0 || bit >= BIT_SIZE(&bs->bitset)) {
        raise_exc_fmt("bit index %lld out of range [0, %d)", bit, BIT_SIZE(&bs->bitset));
        return error_value;
    }

    return bool_value(bitset_test(&bs->bitset, (int)bit));
}

// flip(bit int)
static TValue _bitset_flip(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs = SELF_AS(bitset_type);
    int64_t bit = kl_arg_int64(0);

    if (bit < 0 || bit >= BIT_SIZE(&bs->bitset)) {
        raise_exc_fmt("bit index %lld out of range [0, %d)", bit, BIT_SIZE(&bs->bitset));
        return error_value;
    }

    // Flip: test and toggle
    if (bitset_test(&bs->bitset, (int)bit)) {
        bitset_clear(&bs->bitset, (int)bit);
    } else {
        bitset_set(&bs->bitset, (int)bit);
    }

    return no_value;
}

// count() -> int
static TValue _bitset_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    BitSetObject *bs = SELF_AS(bitset_type);

    int count = 0;
    for (int i = 0; i < BIT_SIZE(&bs->bitset); i++) {
        if (bitset_test(&bs->bitset, i)) {
            count++;
        }
    }

    return int64_value(count);
}

// size() -> int
static TValue _bitset_size(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    BitSetObject *bs = SELF_AS(bitset_type);
    return int64_value(BIT_SIZE(&bs->bitset));
}

// __bitand__(other BitSet) -> BitSet
static TValue _bitset_bitand(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs1 = SELF_AS(bitset_type);
    BitSetObject *bs2 = kl_arg_obj_as(0, bitset_type);

    if (BIT_SIZE(&bs1->bitset) != BIT_SIZE(&bs2->bitset)) {
        raise_exc_fmt("BitSet size mismatch: %d vs %d", BIT_SIZE(&bs1->bitset),
                      BIT_SIZE(&bs2->bitset));
        return error_value;
    }

    BitSetObject *result = (BitSetObject *)kl_new_bitset(BIT_SIZE(&bs1->bitset));

    int nrows = (BIT_SIZE(&bs1->bitset) + ROW_SIZE - 1) / ROW_SIZE;
    for (int i = 0; i < nrows; i++) {
        for (int bit = 0; bit < (int)ROW_SIZE; bit++) {
            int idx = i * ROW_SIZE + bit;
            if (idx >= BIT_SIZE(&bs1->bitset)) break;
            if (bitset_test(&bs1->bitset, idx) && bitset_test(&bs2->bitset, idx)) {
                bitset_set(&result->bitset, idx);
            }
        }
    }

    return obj_value((Object *)result);
}

// __bitor__(other BitSet) -> BitSet
static TValue _bitset_bitor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs1 = SELF_AS(bitset_type);
    BitSetObject *bs2 = kl_arg_obj_as(0, bitset_type);

    if (BIT_SIZE(&bs1->bitset) != BIT_SIZE(&bs2->bitset)) {
        raise_exc_fmt("BitSet size mismatch: %d vs %d", BIT_SIZE(&bs1->bitset),
                      BIT_SIZE(&bs2->bitset));
        return error_value;
    }

    BitSetObject *result = (BitSetObject *)kl_new_bitset(BIT_SIZE(&bs1->bitset));

    for (int i = 0; i < BIT_SIZE(&bs1->bitset); i++) {
        if (bitset_test(&bs1->bitset, i) || bitset_test(&bs2->bitset, i)) {
            bitset_set(&result->bitset, i);
        }
    }

    return obj_value((Object *)result);
}

// __bitxor__(other BitSet) -> BitSet
static TValue _bitset_bitxor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BitSetObject *bs1 = SELF_AS(bitset_type);
    BitSetObject *bs2 = kl_arg_obj_as(0, bitset_type);

    if (BIT_SIZE(&bs1->bitset) != BIT_SIZE(&bs2->bitset)) {
        raise_exc_fmt("BitSet size mismatch: %d vs %d", BIT_SIZE(&bs1->bitset),
                      BIT_SIZE(&bs2->bitset));
        return error_value;
    }

    BitSetObject *result = (BitSetObject *)kl_new_bitset(BIT_SIZE(&bs1->bitset));

    for (int i = 0; i < BIT_SIZE(&bs1->bitset); i++) {
        int v1 = bitset_test(&bs1->bitset, i);
        int v2 = bitset_test(&bs2->bitset, i);
        if (v1 != v2) {
            bitset_set(&result->bitset, i);
        }
    }

    return obj_value((Object *)result);
}

// __bitnot__() -> BitSet
static TValue _bitset_bitnot(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    BitSetObject *bs = SELF_AS(bitset_type);

    BitSetObject *result = (BitSetObject *)kl_new_bitset(BIT_SIZE(&bs->bitset));

    // Invert each bit
    for (int i = 0; i < BIT_SIZE(&bs->bitset); i++) {
        if (!bitset_test(&bs->bitset, i)) {
            bitset_set(&result->bitset, i);
        }
    }

    return obj_value((Object *)result);
}

// to_bytes() -> bytes
static TValue _bitset_to_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    BitSetObject *bs = SELF_AS(bitset_type);

    // Create a new BytesObject from the bitset data
    int nrows = (BIT_SIZE(&bs->bitset) + ROW_SIZE - 1) / ROW_SIZE;
    return obj_value(kl_bytes_from_data(bs->bitset.bits, nrows));
}

// from_bytes(data bytes) -> BitSet (static method)
static TValue _bitset_from_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BytesObject *data = kl_arg_obj_as(0, bytes_type);

    int size = BYTES_SIZE(data) * 8;
    BitSetObject *bs = (BitSetObject *)kl_new_bitset(size);
    memcpy(bs->bitset.bits, BYTES_DATA(data), BYTES_SIZE(data));

    return obj_value((Object *)bs);
}

// fmt(f Formatter)
static TValue _bitset_fmt(TValue *self, TValue *args, int nargs)
{
    BitSetObject *bs = SELF_AS(bitset_type);
    ASSERT(nargs == 1);

    Formatter *fmt = kl_arg_fmt(0);
    int size = BIT_SIZE(&bs->bitset);

    buf_write_str(&fmt->buf, "BitSet(nbits=");

    // Write size as integer
    char size_buf[16];
    sprintf(size_buf, "%d", size);
    buf_write_str(&fmt->buf, size_buf);
    buf_write_str(&fmt->buf, ", bits=");

    int limit = size <= 16 ? size : 16;
    for (int i = 0; i < limit; i++) {
        buf_write_char(&fmt->buf, bitset_test(&bs->bitset, i) ? '1' : '0');
    }

    if (size > 16) {
        buf_write_str(&fmt->buf, "...");
    }

    buf_write_char(&fmt->buf, ')');

    return no_value;
}

// dump() -> str
static TValue _bitset_dump(TValue *self, TValue *args, int nargs)
{
    BitSetObject *bs = SELF_AS(bitset_type);
    ASSERT(nargs == 0);

    int size = BIT_SIZE(&bs->bitset);
    BUF(buf);

    for (int i = 0; i < size; i++) {
        buf_write_char(&buf, bitset_test(&bs->bitset, i) ? '1' : '0');

        // Add space after every 8 bits
        if ((i + 1) % 8 == 0 && i + 1 < size) {
            buf_write_char(&buf, ' ');
        }
        // Add newline after every 32 bits (4 groups of 8)
        if ((i + 1) % 32 == 0 && i + 1 < size) {
            buf_write_char(&buf, '\n');
        }
    }

    Object *s = kl_new_str(BUF_STR(buf));

    FINI_BUF(buf);

    return obj_value(s);
}

static MethodDef bitset_methods[] = {
    { "__init__", _bitset_init },
    { "fmt", _bitset_fmt },
    { "dump", _bitset_dump },
    { "set", _bitset_set },
    { "clear", _bitset_clear },
    { "test", _bitset_test },
    { "flip", _bitset_flip },
    { "count", _bitset_count },
    { "size", _bitset_size },
    { "__bitand__", _bitset_bitand },
    { "__bitor__", _bitset_bitor },
    { "__bitxor__", _bitset_bitxor },
    { "__bitnot__", _bitset_bitnot },
    { "to_bytes", _bitset_to_bytes },
    { "from_bytes", _bitset_from_bytes },
    { NULL },
};

TypeObject bitset_type = {
    ._type = &type_type,
    .name = "BitSet",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(BitSetObject),
    .methdefs = bitset_methods,
};

Object *kl_new_bitset(int nbits)
{
    BitSetObject *bs = mm_alloc_obj(bs);
    INIT_OBJECT_HEAD(bs, &bitset_type, 0);
    init_bitset(&bs->bitset, nbits);
    return (Object *)bs;
}

#ifdef __cplusplus
}
#endif
