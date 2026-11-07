/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Decompress: the object `zlib.decompressobj()` returns.
 *
 * CPython's type is `zlib.Decompress` and the three attributes Koala has to
 * keep in step with the engine are plain fields, not methods:
 *
 *     d.eof              bool   - the end of the stream has been reached
 *     d.unused_data      bytes  - data found after the end of the stream
 *     d.unconsumed_tail  bytes  - input the last `decompress()` did not use
 *
 * `unused_data` and `unconsumed_tail` are read-only in Python; here they are
 * `pub var` because the runtime has no computed properties, and the engine
 * writes them from C.
 */

#include "zlib_shim.h"

typedef struct _DecompressObject {
    OBJECT_HEAD
    /* Field 0: end of stream reached. */
    TValue eof;
    /* Field 1: bytes after the end of the stream. */
    TValue unused_data;
    /* Field 2: input the last decompress() call did not consume. */
    TValue unconsumed_tail;
    z_stream zst;
    int started;
    /* Accumulator behind `unused_data`. Never reset: the field grows. */
    Buffer unused;
    /* Preset dictionary, applied when the stream asks for it (Z_NEED_DICT).
     * A zlib-wrapped stream carries the "dictionary needed" flag in its
     * header, so the dictionary cannot be installed before inflate() has read
     * that header - CPython stores it and applies it on demand too. */
    Buffer dict;
} DecompressObject;

static TypeObject decompress_type;

#define DEC(self) ((DecompressObject *)SELF_AS(decompress_type))

static TValue _empty_bytes(void) { return obj_value(kl_bytes_from_data(NULL, 0)); }

/* Refresh a `bytes` field from its accumulator. The accumulator keeps its
 * contents, so `unused_data` really accumulates across calls. */
static void _publish(Buffer *buf, TValue *field)
{
    *field = obj_value(kl_bytes_from_data((uint8_t *)buf->buf, (uint32_t)buf->len));
}

/* Publish the input the last call did not consume. CPython keeps the leftover
 * in this attribute rather than inside the engine, so a caller that feeds the
 * tail back - the documented loop - resumes exactly where it stopped. */
static void _set_tail(DecompressObject *d, const uint8_t *p, size_t n)
{
    d->unconsumed_tail = obj_value(kl_bytes_from_data((uint8_t *)p, (uint32_t)n));
}

static void _end_stream(DecompressObject *d)
{
    if (d->started) {
        inflateEnd(&d->zst);
        d->started = 0;
    }
}

static void _decompress_free(Object *self)
{
    DecompressObject *d = (DecompressObject *)self;

    _end_stream(d);
    FINI_BUF(d->unused);
    FINI_BUF(d->dict);
}

// __init__(wbits int, zdict bytes? = nil)
static TValue _decompress_init(TValue *self, TValue *args, int nargs)
{
    DecompressObject *d = DEC(self);
    int wbits, err;

    ASSERT(nargs == 2);
    wbits = (int)kl_arg_int(0);

    memset(&d->zst, 0, sizeof(d->zst));
    INIT_BUF(d->unused);
    INIT_BUF(d->dict);
    d->started = 0;
    d->eof = bool_value(0);
    d->unused_data = _empty_bytes();
    d->unconsumed_tail = _empty_bytes();

    err = inflateInit2(&d->zst, wbits);
    if (err == Z_MEM_ERROR) {
        raise_exc_str("zlib: out of memory");
        return error_value;
    } else if (err != Z_OK) {
        raise_exc_str("zlib: Invalid initialization option");
        return error_value;
    }
    d->started = 1;

    if (!is_nil(args + 1)) {
        const uint8_t *dict;
        uint32_t n;

        if (!zlib_arg_bytes(args + 1, &dict, &n)) {
            _end_stream(d);
            return error_value;
        }

        if (n > 0 && !zlib_buf_write(&d->dict, dict, n)) {
            _end_stream(d);
            return error_value;
        }
    }

    return no_value;
}

/* Install the preset dictionary once inflate() asks for it.
 *
 * Returns 1 when the engine may retry, 0 after raising. */
static int _apply_dict(DecompressObject *d)
{
    int err;

    if (BUF_LEN(d->dict) == 0) {
        zlib_raise(&d->zst, Z_NEED_DICT, "decompressing data");
        return 0;
    }

    err = inflateSetDictionary(&d->zst, (const Bytef *)BUF_STR(d->dict), (uInt)BUF_LEN(d->dict));
    if (err != Z_OK) {
        zlib_raise(&d->zst, err, "decompressing data");
        return 0;
    }

    return 1;
}

// decompress(data bytes, max_length int) bytes
static TValue _decompress_data(TValue *self, TValue *args, int nargs)
{
    DecompressObject *d = DEC(self);
    const uint8_t *in;
    uint8_t *base;
    uint32_t n;
    int64_t max_length;
    size_t total, rest_len;
    Buffer inbuf, out;
    int err = Z_OK;

    ASSERT(nargs == 2);
    if (!d->started) {
        raise_exc_str("zlib: Error -2 while decompressing data: inconsistent stream state");
        return error_value;
    }
    if (!zlib_arg_bytes(args + 0, &in, &n)) return error_value;

    max_length = kl_arg_int(1);
    if (max_length < 0) max_length = 0;

    /* Data offered after the end of the stream is not an error: CPython adds
     * it to `unused_data` and returns b"". */
    if (d->eof.ival != 0) {
        if (!zlib_buf_write(&d->unused, in, n)) return error_value;
        _publish(&d->unused, &d->unused_data);
        return _empty_bytes();
    }

    /* CPython hands the leftover input back to the caller in `unconsumed_tail`
     * and expects it to be fed back: the documented loop is
     *
     *     chunk = d.unconsumed_tail
     *
     * so the tail is not held inside the engine a second time. */
    INIT_BUF(inbuf);
    if (!zlib_buf_write(&inbuf, in, n)) {
        FINI_BUF(inbuf);
        return error_value;
    }

    base = (uint8_t *)BUF_STR(inbuf);
    total = (size_t)BUF_LEN(inbuf);
    d->zst.next_in = base;
    d->zst.avail_in = (uInt)total;

    INIT_BUF(out);
    for (;;) {
        size_t want = ZLIB_CHUNK, have;

        if (max_length > 0) {
            size_t left = (size_t)max_length - (size_t)BUF_LEN(out);

            if (left == 0) break;
            if (left < want) want = left;
        }

        {
            uint8_t chunk[ZLIB_CHUNK];

            d->zst.next_out = chunk;
            d->zst.avail_out = (uInt)want;
            err = inflate(&d->zst, Z_NO_FLUSH);
            have = want - d->zst.avail_out;

            if (!zlib_buf_write(&out, chunk, have)) {
                FINI_BUF(inbuf);
                FINI_BUF(out);
                return error_value;
            }
        }

        if (err == Z_STREAM_END) {
            d->eof = bool_value(1);
            break;
        }

        /* A preset dictionary is installed only now, once the header has been
         * read; the retry continues with the same input pointer. */
        if (err == Z_NEED_DICT) {
            if (!_apply_dict(d)) {
                FINI_BUF(inbuf);
                FINI_BUF(out);
                return error_value;
            }
            continue;
        }

        if (err != Z_OK && err != Z_BUF_ERROR) {
            FINI_BUF(inbuf);
            FINI_BUF(out);
            zlib_raise(&d->zst, err, "decompressing data");
            return error_value;
        }

        /* Z_BUF_ERROR (or a call that consumed nothing and produced nothing)
         * means the engine needs more input: the caller has to feed another
         * chunk, and whatever is left in `avail_in` is reported below. */
        if (err == Z_BUF_ERROR || d->zst.avail_in == 0) break;
    }

    rest_len = (size_t)d->zst.avail_in;
    if (d->eof.ival != 0) {
        if (rest_len > 0) {
            if (!zlib_buf_write(&d->unused, base + (total - rest_len), rest_len)) {
                FINI_BUF(inbuf);
                FINI_BUF(out);
                return error_value;
            }
            _publish(&d->unused, &d->unused_data);
        }
        _set_tail(d, NULL, 0);
    } else {
        _set_tail(d, base + (total - rest_len), rest_len);
    }

    FINI_BUF(inbuf);
    return zlib_take_bytes(&out);
}

// flush(length int) bytes
static TValue _decompress_flush(TValue *self, TValue *args, int nargs)
{
    DecompressObject *d = DEC(self);
    int64_t length;
    Buffer out;
    int err = Z_OK;

    ASSERT(nargs == 1);
    if (!d->started) {
        raise_exc_str("zlib: Error -2 while flushing: inconsistent stream state");
        return error_value;
    }
    length = kl_arg_int(0);
    if (length <= 0) length = (int64_t)ZLIB_CHUNK;
    if (length > (int64_t)ZLIB_CHUNK) length = (int64_t)ZLIB_CHUNK;

    /* A finished stream has nothing left to flush - CPython returns b"". */
    if (d->eof.ival != 0) return _empty_bytes();

    INIT_BUF(out);
    d->zst.next_in = Z_NULL;
    d->zst.avail_in = 0;

    for (;;) {
        size_t want = (size_t)length, have;
        uint8_t chunk[ZLIB_CHUNK];

        d->zst.next_out = chunk;
        d->zst.avail_out = (uInt)want;
        err = inflate(&d->zst, Z_FINISH);
        have = want - d->zst.avail_out;

        if (!zlib_buf_write(&out, chunk, have)) {
            FINI_BUF(out);
            return error_value;
        }

        if (err == Z_STREAM_END) {
            d->eof = bool_value(1);
            break;
        }

        if (err == Z_NEED_DICT) {
            if (!_apply_dict(d)) {
                FINI_BUF(out);
                return error_value;
            }
            continue;
        }

        /* Truncated input is not an error here: CPython swallows Z_BUF_ERROR
         * and reports what was produced so far. */
        if (err != Z_OK && err != Z_BUF_ERROR) {
            FINI_BUF(out);
            zlib_raise(&d->zst, err, "flushing");
            return error_value;
        }

        if (d->zst.avail_out != 0) break;
    }

    return zlib_take_bytes(&out);
}

// copy() Decompress
static TValue _decompress_copy(TValue *self, TValue *args, int nargs)
{
    DecompressObject *d = DEC(self);
    DecompressObject *copy;
    Object *obj;
    int err;

    ASSERT(nargs == 0);
    if (!d->started) {
        raise_exc_str("zlib: Error -2 while copying: inconsistent stream state");
        return error_value;
    }

    obj = kl_new_instance(&decompress_type);
    copy = (DecompressObject *)obj;
    memset(&copy->zst, 0, sizeof(copy->zst));
    INIT_BUF(copy->unused);
    INIT_BUF(copy->dict);
    copy->started = 0;
    copy->eof = d->eof;
    copy->unused_data = _empty_bytes();
    copy->unconsumed_tail = d->unconsumed_tail;

    err = inflateCopy(&copy->zst, &d->zst);
    if (err != Z_OK) {
        raise_exc_str("zlib: out of memory");
        return error_value;
    }
    copy->started = 1;

    if (!zlib_buf_write(&copy->unused, BUF_STR(d->unused), BUF_LEN(d->unused)) ||
        !zlib_buf_write(&copy->dict, BUF_STR(d->dict), BUF_LEN(d->dict))) {
        return error_value;
    }
    _publish(&copy->unused, &copy->unused_data);

    return obj_value(obj);
}

// _close()
static TValue _decompress_close(TValue *self, TValue *args, int nargs)
{
    DecompressObject *d = DEC(self);

    ASSERT(nargs == 0);
    _end_stream(d);
    return no_value;
}

static MethodDef decompress_methods[] = {
    { "__init__", _decompress_init },
    { "decompress", _decompress_data },
    { "flush", _decompress_flush },
    { "copy", _decompress_copy },
    { "_close", _decompress_close },
    { NULL, NULL },
};

static TypeObject decompress_type = {
    ._type = &type_type,
    ._nfields = 3,
    .name = "Decompress",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(DecompressObject),
    .methdefs = decompress_methods,
    ._free = _decompress_free,
};

void zlib_reg_decompress(NativeLib *lib) { kl_reg_type(lib, &decompress_type); }