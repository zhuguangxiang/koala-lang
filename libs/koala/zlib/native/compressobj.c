/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Compress: the object `zlib.compressobj()` returns.
 *
 * CPython calls the type `zlib.Compress`, and the members are `compress()`,
 * `flush()` and `copy()` - the same three, with the same defaults, so code
 * that swaps `import zlib` for `import "koala/zlib"` keeps working.
 *
 * One member is Koala-only and deliberately underscore-prefixed: `_close()`
 * releases the engine state for the library's own `gzip`/`zipfile` code,
 * which builds a stream per chunk and cannot wait for a finalizer (the
 * runtime does not run `TypeObject._free` yet, see `kl_free_instance()`).
 */

#include "zlib_shim.h"

typedef struct _CompressObject {
    OBJECT_HEAD
    z_stream zst;
    /* 1 once deflateInit2() succeeded, so the end call cannot run twice. */
    int started;
    /* 1 once the stream was finished with Z_FINISH: the engine then rejects
     * both more input and another flush, and CPython reports that as a
     * state error rather than a broken stream. */
    int finished;
    /* Output collected between calls; emptied by every return. */
    Buffer out;
} CompressObject;

static TypeObject compress_type;

#define COMP(self) ((CompressObject *)SELF_AS(compress_type))

static void _end_stream(CompressObject *co)
{
    if (co->started) {
        deflateEnd(&co->zst);
        co->started = 0;
    }
}

static void _compress_free(Object *self)
{
    CompressObject *co = (CompressObject *)self;

    _end_stream(co);
    FINI_BUF(co->out);
}

/* Raise the message CPython's `Compress.__init__` raises for a bad option. */
static void _raise_init_error(int err)
{
    if (err == Z_MEM_ERROR) {
        raise_exc_str("zlib: out of memory");
    } else if (err == Z_STREAM_ERROR) {
        raise_exc_str("zlib: Bad compression level");
    } else {
        raise_exc_str("zlib: Invalid initialization option");
    }
}

// __init__(level int, method int, wbits int, memLevel int, strategy int,
//          zdict bytes? = nil)
static TValue _compress_init(TValue *self, TValue *args, int nargs)
{
    CompressObject *co = COMP(self);
    int level, method, wbits, memlevel, strategy, err;

    ASSERT(nargs == 6);
    level = (int)kl_arg_int(0);
    method = (int)kl_arg_int(1);
    wbits = (int)kl_arg_int(2);
    memlevel = (int)kl_arg_int(3);
    strategy = (int)kl_arg_int(4);

    memset(&co->zst, 0, sizeof(co->zst));
    INIT_BUF(co->out);
    co->started = 0;
    co->finished = 0;

    err = deflateInit2(&co->zst, level, method, wbits, memlevel, strategy);
    if (err != Z_OK) {
        _raise_init_error(err);
        return error_value;
    }
    co->started = 1;

    if (!is_nil(args + 5)) {
        const uint8_t *dict;
        uint32_t n;

        if (!zlib_arg_bytes(args + 5, &dict, &n)) {
            _end_stream(co);
            return error_value;
        }

        if (n > 0) {
            err = deflateSetDictionary(&co->zst, dict, (uInt)n);
            if (err != Z_OK) {
                _end_stream(co);
                _raise_init_error(err);
                return error_value;
            }
        }
    }

    return no_value;
}

// compress(data bytes) bytes
static TValue _compress_data(TValue *self, TValue *args, int nargs)
{
    CompressObject *co = COMP(self);
    const uint8_t *in;
    uint32_t n;
    int err;

    ASSERT(nargs == 1);
    if (!co->started || co->finished) {
        raise_exc_str("zlib: Error -2 while compressing data: inconsistent stream state");
        return error_value;
    }
    if (!zlib_arg_bytes(args + 0, &in, &n)) return error_value;

    co->zst.next_in = (Bytef *)in;
    co->zst.avail_in = (uInt)n;

    do {
        uint8_t chunk[ZLIB_CHUNK];
        size_t have;

        co->zst.next_out = chunk;
        co->zst.avail_out = (uInt)ZLIB_CHUNK;
        err = deflate(&co->zst, Z_NO_FLUSH);
        /* CPython lets every code but Z_STREAM_ERROR through here: a full
         * output window is not a failure, it is the next iteration. */
        if (err == Z_STREAM_ERROR) {
            zlib_raise(&co->zst, err, "compressing data");
            return error_value;
        }

        have = ZLIB_CHUNK - co->zst.avail_out;
        if (!zlib_buf_write(&co->out, chunk, have)) return error_value;
    } while (co->zst.avail_in > 0);

    return zlib_take_bytes(&co->out);
}

// flush(mode int) bytes
static TValue _compress_flush(TValue *self, TValue *args, int nargs)
{
    CompressObject *co = COMP(self);
    int mode, err;

    ASSERT(nargs == 1);
    if (!co->started || co->finished) {
        raise_exc_str("zlib: Error -2 while flushing: inconsistent stream state");
        return error_value;
    }
    mode = (int)kl_arg_int(0);

    /* The mode is handed to zlib unchecked, exactly as CPython does, so an
     * out-of-range value produces zlib's own "inconsistent stream state". */
    co->zst.next_in = Z_NULL;
    co->zst.avail_in = 0;

    do {
        uint8_t chunk[ZLIB_CHUNK];
        size_t have;

        co->zst.next_out = chunk;
        co->zst.avail_out = (uInt)ZLIB_CHUNK;
        err = deflate(&co->zst, mode);
        if (err != Z_OK && err != Z_STREAM_END) {
            zlib_raise(&co->zst, err, "flushing");
            return error_value;
        }

        have = ZLIB_CHUNK - co->zst.avail_out;
        if (!zlib_buf_write(&co->out, chunk, have)) return error_value;

        /* Z_FINISH ends the stream: nothing may be compressed or flushed
         * afterwards, exactly as in Python. */
        if (err == Z_STREAM_END) {
            co->finished = 1;
            break;
        }
    } while (co->zst.avail_out == 0);

    return zlib_take_bytes(&co->out);
}

// copy() Compress
static TValue _compress_copy(TValue *self, TValue *args, int nargs)
{
    CompressObject *co = COMP(self);
    CompressObject *copy;
    Object *obj;
    int err;

    ASSERT(nargs == 0);
    if (!co->started || co->finished) {
        raise_exc_str("zlib: Inconsistent stream state");
        return error_value;
    }

    obj = kl_new_instance(&compress_type);
    copy = (CompressObject *)obj;
    memset(&copy->zst, 0, sizeof(copy->zst));
    INIT_BUF(copy->out);
    copy->started = 0;
    copy->finished = 0;

    err = deflateCopy(&copy->zst, &co->zst);
    if (err != Z_OK) {
        raise_exc_str("zlib: out of memory");
        return error_value;
    }
    copy->started = 1;

    return obj_value(obj);
}

// _close()
static TValue _compress_close(TValue *self, TValue *args, int nargs)
{
    CompressObject *co = COMP(self);

    ASSERT(nargs == 0);
    _end_stream(co);
    return no_value;
}

static MethodDef compress_methods[] = {
    { "__init__", _compress_init },
    { "compress", _compress_data },
    { "flush", _compress_flush },
    { "copy", _compress_copy },
    { "_close", _compress_close },
    { NULL, NULL },
};

static TypeObject compress_type = {
    ._type = &type_type,
    ._nfields = 0,
    .name = "Compress",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(CompressObject),
    .methdefs = compress_methods,
    ._free = _compress_free,
};

void zlib_reg_compress(NativeLib *lib) { kl_reg_type(lib, &compress_type); }