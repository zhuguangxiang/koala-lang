/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Koala bindings for the RFC 4648 section 8 hexadecimal codec. The shape
 * mirrors base64_shim.c: `encode` renders bytes as lowercase text and
 * `decode` accepts digits of either case. The codec itself is in
 * hex_core.c, which has no runtime dependency and is tested against
 * CPython's binascii out of tree.
 */

#include <stdlib.h>
#include "bytesobj.h"
#include "excobj.h"
#include "hex_core.h"

/* Turn a HEX_ERR_* code into the message the Koala side panics with. */
static void _raise_decode_error(ptrdiff_t err)
{
    if (err == HEX_ERR_ODD) {
        raise_exc_str("invalid hex string: odd number of digits");
    } else {
        raise_exc_str("invalid hex string: not a hexadecimal digit");
    }
}

// func encode(b bytes) str {}
static TValue _hex_encode(TValue *self, TValue *args, int nargs)
{
    BytesObject *bs;
    size_t n, olen;
    uint8_t *buf;
    TValue res;

    ASSERT(nargs == 1);
    bs = kl_arg_obj_as(0, bytes_type);
    n = BYTES_SIZE(bs);

    /* The Koala side declares a str result, so the buffer is copied into a
     * string and then released. */
    olen = hex_encoded_len(n);
    buf = malloc(olen ? olen : 1);
    if (!buf) {
        raise_exc_str("hex: out of memory");
        return error_value;
    }

    hex_encode(BYTES_DATA(bs) + BYTES_OFFSET(bs), n, buf, hex_digits(0));
    res = kl_val_nstr((char *)buf, olen);
    free(buf);

    return res;
}

// func decode(s str) bytes {}
static TValue _hex_decode(TValue *self, TValue *args, int nargs)
{
    Object *so, *res;
    size_t n, max;
    uint8_t *buf;
    ptrdiff_t r;

    ASSERT(nargs == 1);
    so = kl_arg_strobj(0);
    n = STR_LEN(so);

    max = n / 2;
    buf = malloc(max ? max : 1);
    if (!buf) {
        raise_exc_str("hex: out of memory");
        return error_value;
    }

    r = hex_decode((const uint8_t *)STR_BUF(so), n, buf);
    if (r < 0) {
        free(buf);
        _raise_decode_error(r);
        return error_value;
    }

    res = kl_bytes_from_data(buf, (uint32_t)r);
    free(buf);
    return obj_value(res);
}

/// Entry point called by the runtime after `dlopen("libkoala_hex.so")`.
///
/// The name must be `<last segment of the package path>_native_lib_init`,
/// so `koala/hex` becomes `hex_native_lib_init`.
void hex_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "encode", _hex_encode);
    kl_reg_func(lib, "decode", _hex_decode);
}
