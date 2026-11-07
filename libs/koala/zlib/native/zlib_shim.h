/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Shared helpers for the koala/zlib native module.
 *
 * One C file per public class, the way CPython's `Modules/zlibmodule.c`
 * does it:
 *   - zlib_shim.c:     module init, crc32/adler32, compress/decompress
 *   - compressobj.c:   Compress   (behind `zlib.compressobj()`)
 *   - decompressobj.c: Decompress (behind `zlib.decompressobj()`)
 *
 * The DEFLATE engine is the official zlib 1.3.1 in `native/zlib/`, copied
 * unmodified from the upstream release (see `native/zlib/VERSION` and
 * `native/zlib/LICENSE`). `zconf.h` and `zlib.h` are upstream too, so the
 * API types here are exactly the ones CPython compiles against - which is
 * why every error message below can be spelled the same way CPython spells
 * it: the text comes from zlib itself, not from us.
 */

#ifndef KOALA_ZLIB_SHIM_H
#define KOALA_ZLIB_SHIM_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "buffer.h"
#include "bytesobj.h"
#include "excobj.h"
#include "object.h"

/* The chunk size `decompress()` and `flush()` feed the engine with. CPython
 * uses the same default (`zlib.DEF_BUF_SIZE`). */
#define ZLIB_CHUNK ((size_t)16384)

/* `DEF_MEM_LEVEL` is defined in zlib's internal `zutil.h`, not in the public
 * `zlib.h` (which only exposes `MAX_MEM_LEVEL`); 8 is zlib 1.3.1's value, and
 * the same one `zlib.DEF_MEM_LEVEL` reports in Python. */
#ifndef DEF_MEM_LEVEL
#define DEF_MEM_LEVEL 8
#endif

/* Raise the panic CPython's zlib module raises for a failed engine call.
 *
 * CPython's `zlib_error()` formats "Error %d while %s: %s", taking the
 * trailing text from `zst->msg` when zlib set one and filling in its own
 * wording otherwise; reproducing that mapping keeps the messages identical
 * for both the zlib-generated and the swallowed cases. */
static inline void zlib_raise(z_stream *zst, int err, const char *what)
{
    const char *zmsg = zst->msg;

    if (zmsg == NULL) {
        switch (err) {
            case Z_VERSION_ERROR:
                zmsg = "incompatible version";
                break;
            case Z_BUF_ERROR:
                if (zst->avail_in == 0) {
                    zmsg = "incomplete or truncated stream";
                    break;
                }
                /* fall through: input left over means a bad call sequence */
            case Z_STREAM_ERROR:
                zmsg = "inconsistent stream state";
                break;
            default:
                zmsg = "";
                break;
        }
    }

    raise_exc_fmt("zlib: Error %d while %s: %s", err, what, zmsg);
}

/* Read a `bytes` argument.
 *
 * Returns 1 and fills `p`/`n` on success; `n` is 0 for the empty `bytes`. The
 * Koala declaration types the parameter `bytes`, so a `str` is rejected by the
 * compiler exactly where CPython's TypeError would fire. */
static inline int zlib_arg_bytes(TValue *v, const uint8_t **p, uint32_t *n)
{
    Object *o = to_obj(v);

    if (!IS_BYTES(o)) {
        raise_exc_str("zlib: a bytes-like object is required");
        return 0;
    }

    *p = BYTES_DATA(o) + BYTES_OFFSET(o);
    *n = BYTES_SIZE(o);
    return 1;
}

/* Append `n` bytes at `p` to `buf`. Returns 0 after raising on OOM. */
static inline int zlib_buf_write(Buffer *buf, const void *p, size_t n)
{
    if (n == 0) return 1;

    if (buf_reserve(buf, n) < 0) {
        raise_exc_str("zlib: out of memory");
        return 0;
    }

    memcpy(buf->buf + buf->len, p, n);
    buf->len += (int)n;
    return 1;
}

/* Hand the accumulated bytes over as a new `bytes` object and reset `buf`.
 *
 * `kl_bytes_from_data` copies, so the buffer keeps its allocation and is
 * reused by the next call - `compressobj.compress()` may run in a loop. */
static inline TValue zlib_take_bytes(Buffer *buf)
{
    Object *res = kl_bytes_from_data((uint8_t *)buf->buf, (uint32_t)buf->len);

    buf->len = 0;
    return obj_value(res);
}

/* Registration entry points, one per C file. */
void zlib_reg_funcs(NativeLib *lib);
void zlib_reg_compress(NativeLib *lib);
void zlib_reg_decompress(NativeLib *lib);

#endif /* KOALA_ZLIB_SHIM_H */