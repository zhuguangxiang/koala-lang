/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * One-shot hash functions, matching the @native declarations in
 * hash.kl. All of them go through EVP_Digest: OpenSSL 3.0 marks the
 * low-level SHA*()/MD5() functions deprecated, and SHA3 only exists
 * through EVP anyway, so one path covers everything.
 */

#include <openssl/evp.h>
#include "bytesobj.h"

// One-shot digest over the bytes argument. `md` is one of the
// EVP_sha1() / EVP_sha256() / ... descriptors; the output length is
// whatever that algorithm produces, read back from `dlen`.
static TValue _evp_digest(const EVP_MD *md, TValue *args, int nargs)
{
    BytesObject *bs = kl_arg_obj_as(0, bytes_type);
    ASSERT(bs != NULL);

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen = 0;
    int ok = EVP_Digest(BYTES_DATA(bs) + BYTES_OFFSET(bs), BYTES_SIZE(bs), digest, &dlen, md, NULL);
    ASSERT(ok == 1);

    Object *res = kl_bytes_from_data(digest, dlen);
    return obj_value(res);
}

// func sha1(data bytes) bytes
static TValue _sha1(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha1(), args, nargs);
}

// func sha224(data bytes) bytes
static TValue _sha224(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha224(), args, nargs);
}

// func sha256(data bytes) bytes
static TValue _sha256(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha256(), args, nargs);
}

// func sha384(data bytes) bytes
static TValue _sha384(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha384(), args, nargs);
}

// func sha512(data bytes) bytes
static TValue _sha512(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha512(), args, nargs);
}

// func sha3_224(data bytes) bytes
static TValue _sha3_224(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha3_224(), args, nargs);
}

// func sha3_256(data bytes) bytes
static TValue _sha3_256(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha3_256(), args, nargs);
}

// func sha3_384(data bytes) bytes
static TValue _sha3_384(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha3_384(), args, nargs);
}

// func sha3_512(data bytes) bytes
static TValue _sha3_512(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_sha3_512(), args, nargs);
}

// func md5(data bytes) bytes
static TValue _md5(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return _evp_digest(EVP_md5(), args, nargs);
}

void crypto_hash_register(NativeLib *lib)
{
    kl_reg_func(lib, "sha1", _sha1);
    kl_reg_func(lib, "sha224", _sha224);
    kl_reg_func(lib, "sha256", _sha256);
    kl_reg_func(lib, "sha384", _sha384);
    kl_reg_func(lib, "sha512", _sha512);
    kl_reg_func(lib, "sha3_224", _sha3_224);
    kl_reg_func(lib, "sha3_256", _sha3_256);
    kl_reg_func(lib, "sha3_384", _sha3_384);
    kl_reg_func(lib, "sha3_512", _sha3_512);
    kl_reg_func(lib, "md5", _md5);
}
