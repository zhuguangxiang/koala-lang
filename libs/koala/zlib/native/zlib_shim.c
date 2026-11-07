/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Module-level zlib functions.
 *
 * The names, the parameter order, the defaults and the failure behaviour are
 * CPython's `zlib` module's: `compress()`, `decompress()`, `crc32()`,
 * `adler32()` plus the two version strings. The heuristic that decides what
 * each zlib return code means is the one in CPython's `zlib_error()`, so a
 * failure reads the same way here as it does in Python.
 */

#include "zlib_shim.h"

/* Initial buffer for `decompress()`, matching CPython's use of `bufsize`. */
#define DECOMPRESS_MIN_BUF ((size_t)1)

// compress(data bytes, level int, wbits int) bytes
static TValue _compress(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    uint32_t n;
    int level, wbits, err;
    z_stream zst;
    Buffer out;

    ASSERT(nargs == 3);
    if (!zlib_arg_bytes(args + 0, &in, &n)) return error_value;
    level = (int)kl_arg_int(1);
    wbits = (int)kl_arg_int(2);

    memset(&zst, 0, sizeof(zst));
    err = deflateInit2(&zst, level, Z_DEFLATED, wbits, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (err == Z_MEM_ERROR) {
        raise_exc_str("zlib: out of memory");
        return error_value;
    } else if (err == Z_STREAM_ERROR) {
        /* CPython's wording for a level or window size zlib refuses. */
        raise_exc_str("zlib: Bad compression level");
        return error_value;
    } else if (err != Z_OK) {
        raise_exc_str("zlib: Invalid initialization option");
        return error_value;
    }

    INIT_BUF(out);
    zst.next_in = (Bytef *)in;
    zst.avail_in = (uInt)n;

    /* One pass is enough for the common case, but a full output window is
     * possible, so keep running until the engine reports the stream end. */
    for (;;) {
        uint8_t chunk[ZLIB_CHUNK];
        size_t have;

        zst.next_out = chunk;
        zst.avail_out = (uInt)ZLIB_CHUNK;
        err = deflate(&zst, Z_FINISH);
        if (err != Z_OK && err != Z_STREAM_END) {
            deflateEnd(&zst);
            FINI_BUF(out);
            zlib_raise(&zst, err, "compressing data");
            return error_value;
        }

        have = ZLIB_CHUNK - zst.avail_out;
        if (!zlib_buf_write(&out, chunk, have)) {
            deflateEnd(&zst);
            FINI_BUF(out);
            return error_value;
        }

        if (err == Z_STREAM_END) break;
    }

    deflateEnd(&zst);
    return zlib_take_bytes(&out);
}

// decompress(data bytes, wbits int, bufsize int) bytes
static TValue _decompress(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    uint32_t n;
    int wbits, err;
    int64_t bufsize;
    z_stream zst;
    Buffer out;

    ASSERT(nargs == 3);
    if (!zlib_arg_bytes(args + 0, &in, &n)) return error_value;
    wbits = (int)kl_arg_int(1);
    bufsize = kl_arg_int(2);
    if (bufsize < (int64_t)DECOMPRESS_MIN_BUF) bufsize = (int64_t)DECOMPRESS_MIN_BUF;

    memset(&zst, 0, sizeof(zst));
    err = inflateInit2(&zst, wbits);
    if (err == Z_MEM_ERROR) {
        raise_exc_str("zlib: out of memory");
        return error_value;
    } else if (err != Z_OK) {
        raise_exc_str("zlib: Invalid initialization option");
        return error_value;
    }

    INIT_BUF(out);
    zst.next_in = (Bytef *)in;
    zst.avail_in = (uInt)n;

    for (;;) {
        Buffer chunk;
        size_t want = (size_t)bufsize, have;

        INIT_BUF(chunk);
        if (buf_reserve(&chunk, want + 1) < 0) {
            inflateEnd(&zst);
            FINI_BUF(out);
            raise_exc_str("zlib: out of memory");
            return error_value;
        }

        zst.next_out = (Bytef *)BUF_STR(chunk);
        zst.avail_out = (uInt)want;
        err = inflate(&zst, Z_FINISH);
        have = want - zst.avail_out;

        if (!zlib_buf_write(&out, BUF_STR(chunk), have)) {
            FINI_BUF(chunk);
            inflateEnd(&zst);
            FINI_BUF(out);
            return error_value;
        }
        FINI_BUF(chunk);

        if (err == Z_STREAM_END) break;

        if (err != Z_OK && err != Z_BUF_ERROR) {
            inflateEnd(&zst);
            FINI_BUF(out);
            zlib_raise(&zst, err, "decompressing data");
            return error_value;
        }

        /* Z_OK with the whole input consumed means the input ended before the
         * stream did; Z_BUF_ERROR means the engine made no progress at all.
         * Both are the truncated-stream case in CPython. */
        if (zst.avail_in == 0 && have == 0) {
            int code = (err == Z_OK) ? Z_BUF_ERROR : err;

            inflateEnd(&zst);
            FINI_BUF(out);
            zlib_raise(&zst, code, "decompressing data");
            return error_value;
        }
    }

    inflateEnd(&zst);
    return zlib_take_bytes(&out);
}

// crc32(data bytes, value int) int
static TValue _crc32(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    uint32_t n;
    uLong crc;
    uint32_t left;
    const Bytef *p;

    ASSERT(nargs == 2);
    if (!zlib_arg_bytes(args + 0, &in, &n)) return error_value;

    /* CPython reads the seed with `PyLong_AsUnsignedLongMask`, so a negative
     * or oversized value wraps around instead of failing. */
    crc = (uLong)(uint32_t)(uint64_t)kl_arg_int(1);

    /* zlib's crc32() takes a uInt length, so walk the buffer in chunks. */
    left = n;
    p = (const Bytef *)in;
    while (left > 0) {
        uInt step = (left > UINT_MAX) ? UINT_MAX : (uInt)left;

        crc = crc32(crc, p, step);
        p += step;
        left -= step;
    }
    if (n == 0) crc = crc32(crc, Z_NULL, 0);

    return int64_value((int64_t)(uint32_t)crc);
}

// adler32(data bytes, value int) int
static TValue _adler32(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    uint32_t n;
    uLong adler;
    uint32_t left;
    const Bytef *p;

    ASSERT(nargs == 2);
    if (!zlib_arg_bytes(args + 0, &in, &n)) return error_value;

    adler = (uLong)(uint32_t)(uint64_t)kl_arg_int(1);

    left = n;
    p = (const Bytef *)in;
    while (left > 0) {
        uInt step = (left > UINT_MAX) ? UINT_MAX : (uInt)left;

        adler = adler32(adler, p, step);
        p += step;
        left -= step;
    }
    if (n == 0) adler = adler32(adler, Z_NULL, 0);

    return int64_value((int64_t)(uint32_t)adler);
}

// _compile_version() str
static TValue _compile_version(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return kl_val_str(ZLIB_VERSION);
}

// _runtime_version() str
static TValue _runtime_version(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return kl_val_str((char *)zlibVersion());
}

void zlib_reg_funcs(NativeLib *lib)
{
    kl_reg_func(lib, "compress", _compress);
    kl_reg_func(lib, "decompress", _decompress);
    kl_reg_func(lib, "crc32", _crc32);
    kl_reg_func(lib, "adler32", _adler32);
    kl_reg_func(lib, "_compile_version", _compile_version);
    kl_reg_func(lib, "_runtime_version", _runtime_version);
}

/// Entry point called by the runtime after `dlopen("libkoala_zlib.so")`.
///
/// The name must be `<last segment of the package path>_native_lib_init`,
/// so `koala/zlib` becomes `zlib_native_lib_init`.
void zlib_native_lib_init(NativeLib *lib)
{
    zlib_reg_funcs(lib);
    zlib_reg_compress(lib);
    zlib_reg_decompress(lib);
}