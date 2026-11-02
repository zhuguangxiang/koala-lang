/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * One-shot HMAC and constant-time digest comparison, matching the
 * @native declarations in hmac.kl.
 *
 * HMAC goes through EVP_Q_mac rather than the HMAC() one-shot:
 * OpenSSL 3.0 deprecates the low-level HMAC() API, and EVP_Q_mac is
 * the single call that replaces it. Requires OpenSSL 3.0 or newer.
 */

#include <openssl/evp.h>
#include "bytesobj.h"

// Shared body: run HMAC with the named digest ("SHA1"/"SHA256"/
// "SHA512") over key=args[0], msg=args[1].
//
// EVP_Q_mac returns `out` on success and NULL on failure; on failure
// `outlen` is unspecified, so the return value, not `outlen`, is what
// to check.
static TValue _hmac(const char *digest_name, TValue *args, int nargs)
{
    BytesObject *key = kl_arg_obj_as(0, bytes_type);
    BytesObject *msg = kl_arg_obj_as(1, bytes_type);
    ASSERT(key != NULL && msg != NULL);

    unsigned char out[EVP_MAX_MD_SIZE];
    size_t outlen = 0;
    unsigned char *ok = EVP_Q_mac(
        NULL, "HMAC", NULL, digest_name, NULL, BYTES_DATA(key) + BYTES_OFFSET(key), BYTES_SIZE(key),
        BYTES_DATA(msg) + BYTES_OFFSET(msg), BYTES_SIZE(msg), out, sizeof(out), &outlen);
    ASSERT(ok != NULL);
    ASSERT(outlen > 0 && outlen <= sizeof(out));

    Object *res = kl_bytes_from_data(out, (int)outlen);
    return obj_value(res);
}

// func hmac_sha1(key bytes, msg bytes) bytes
static TValue _hmac_sha1(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    return _hmac("SHA1", args, nargs);
}

// func hmac_sha256(key bytes, msg bytes) bytes
static TValue _hmac_sha256(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    return _hmac("SHA256", args, nargs);
}

// func hmac_sha512(key bytes, msg bytes) bytes
static TValue _hmac_sha512(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    return _hmac("SHA512", args, nargs);
}

// func compare_digest(a bytes, b bytes) bool
static TValue _compare_digest(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    BytesObject *a = kl_arg_obj_as(0, bytes_type);
    BytesObject *b = kl_arg_obj_as(1, bytes_type);
    ASSERT(a != NULL && b != NULL);

    size_t alen = BYTES_SIZE(a);
    size_t blen = BYTES_SIZE(b);
    if (alen != blen) {
        return bool_value(false);
    }
    int rc = CRYPTO_memcmp(BYTES_DATA(a) + BYTES_OFFSET(a), BYTES_DATA(b) + BYTES_OFFSET(b), alen);
    return bool_value(rc == 0);
}

void crypto_hmac_register(NativeLib *lib)
{
    kl_reg_func(lib, "hmac_sha1", _hmac_sha1);
    kl_reg_func(lib, "hmac_sha256", _hmac_sha256);
    kl_reg_func(lib, "hmac_sha512", _hmac_sha512);
    kl_reg_func(lib, "compare_digest", _compare_digest);
}
