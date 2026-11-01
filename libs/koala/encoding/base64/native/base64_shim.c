/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Koala bindings for the RFC 4648 base64 codec. The public API and the
 * failure behaviour mirror CPython's `base64` module, with the redundant
 * `b64` prefix dropped because the package name already says base64:
 *
 *     encode(s, altchars=None)
 *     decode(s, altchars=None, validate=False)
 *     encodebytes(s)
 *
 * `None` maps to Koala's `nil`, and the `bytes | str` parameters accept
 * both types the way CPython's `_bytes_from_decode_data` does. The codec
 * itself is in base64_core.c, which has no runtime dependency and is
 * fuzz-tested against CPython out of tree.
 */

#include <stdlib.h>
#include <string.h>

#include "base64_core.h"

#include "bytesobj.h"
#include "excobj.h"

/// Read a `bytes | str` argument.
///
/// Returns 1 on success, 0 for nil, -1 after raising an exception.
static int _bytes_or_str(TValue *v, const uint8_t **p, size_t *n)
{
    Object *o;

    if (is_nil(v)) return 0;

    o = to_obj(v);
    if (IS_BYTES(o)) {
        *p = BYTES_DATA(o) + BYTES_OFFSET(o);
        *n = BYTES_SIZE(o);
        return 1;
    }

    if (IS_STR(o)) {
        /* CPython encodes a str argument as ASCII and rejects anything else. */
        const char *s = STR_BUF(o);
        size_t len = STR_LEN(o), i;

        for (i = 0; i < len; i++) {
            if ((unsigned char)s[i] > 0x7f) {
                raise_exc_str("base64: string argument should contain only ASCII characters");
                return -1;
            }
        }

        *p = (uint8_t *)s;
        *n = len;
        return 1;
    }

    raise_exc_str("base64: expected bytes or str");
    return -1;
}

/// Read the `altchars` argument: nil, or exactly two bytes.
///
/// Returns 1 and fills `out`, 0 for nil, -1 after raising an exception.
static int _alt_of(TValue *v, uint8_t out[2])
{
    const uint8_t *p;
    size_t n;
    int r = _bytes_or_str(v, &p, &n);

    if (r <= 0) return r;
    if (n != 2) {
        raise_exc_str("base64: altchars must be exactly 2 bytes");
        return -1;
    }
    out[0] = p[0];
    out[1] = p[1];
    return 1;
}

/* Copy buf into a fresh bytes object, then release buf. */
static TValue _take_bytes(uint8_t *buf, size_t n)
{
    Object *res = kl_bytes_from_data(buf, (uint32_t)n);

    free(buf);
    return obj_value(res);
}

/* Turn a B64_ERR_* code into the exception CPython raises for it. */
static void _raise_decode_error(ptrdiff_t err)
{
    switch ((int)err) {
    case B64_ERR_INVALID:
        raise_exc_str("base64: number of data characters cannot be 1 more than a multiple of 4");
        break;
    case B64_ERR_EXCESS:
        raise_exc_str("base64: excess padding not allowed");
        break;
    case B64_ERR_CHAR:
        raise_exc_str("base64: only base64 data is allowed");
        break;
    default:
        raise_exc_str("base64: incorrect padding");
        break;
    }
}

static TValue _encode_with(const uint8_t *in, size_t n, const uint8_t *alt)
{
    size_t olen = b64_encoded_len(n);
    uint8_t *buf = malloc(olen ? olen : 1);

    if (!buf) {
        raise_exc_str("base64: out of memory");
        return error_value;
    }

    b64_encode(in, n, buf, alt);
    return _take_bytes(buf, olen);
}

static TValue _decode_with(const uint8_t *in, size_t n, const uint8_t *alt, int strict)
{
    size_t max = b64_decoded_max_len(n);
    uint8_t *buf = malloc(max ? max : 1);
    ptrdiff_t r;

    if (!buf) {
        raise_exc_str("base64: out of memory");
        return error_value;
    }

    r = b64_decode(in, n, buf, alt, strict);
    if (r < 0) {
        free(buf);
        _raise_decode_error(r);
        return error_value;
    }

    return _take_bytes(buf, r);
}

// func encode(s bytes | str, altchars (bytes | str)? = nil) bytes {}
static TValue _encode(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    size_t n;
    uint8_t alt[2];
    int r;

    ASSERT(nargs == 2);
    if (_bytes_or_str(args + 0, &in, &n) <= 0) return error_value;

    r = _alt_of(args + 1, alt);
    if (r < 0) return error_value;

    return _encode_with(in, n, r ? alt : NULL);
}

// func decode(s bytes | str, altchars (bytes | str)? = nil, validate = false) bytes {}
static TValue _decode(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    size_t n;
    uint8_t alt[2];
    int r;

    ASSERT(nargs == 3);
    if (_bytes_or_str(args + 0, &in, &n) <= 0) return error_value;

    r = _alt_of(args + 1, alt);
    if (r < 0) return error_value;

    return _decode_with(in, n, r ? alt : NULL, kl_arg_bool(2));
}

// func encodebytes(s bytes | str) bytes {}
static TValue _encodebytes(TValue *self, TValue *args, int nargs)
{
    const uint8_t *in;
    size_t n, olen;
    uint8_t *buf;
    ptrdiff_t r;

    ASSERT(nargs == 1);
    if (_bytes_or_str(args + 0, &in, &n) <= 0) return error_value;

    olen = b64_encodebytes_len(n);
    buf = malloc(olen ? olen : 1);
    if (!buf) {
        raise_exc_str("base64: out of memory");
        return error_value;
    }

    r = b64_encodebytes(in, n, buf, NULL);
    return _take_bytes(buf, (size_t)r);
}

/// Entry point called by the runtime after `dlopen("libkoala_base64.so")`.
///
/// The name must be `<last segment of the package path>_native_lib_init`,
/// so `koala/encoding/base64` becomes `base64_native_lib_init`.
void base64_native_lib_init(NativeLib *lib)
{
    b64_core_init();

    kl_reg_func(lib, "encode", _encode);
    kl_reg_func(lib, "decode", _decode);
    kl_reg_func(lib, "encodebytes", _encodebytes);
}
