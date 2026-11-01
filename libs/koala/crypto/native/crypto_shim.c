/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Thin shim over libcrypto. Exposes only koala_-prefixed functions so
 * the Koala side never names an OpenSSL type. Built into
 * libkoala_crypto.so, separate from libkoala.so: programs that do not
 * import koala/crypto do not load libcrypto.
 */

#include <openssl/sha.h>
#include "bytesobj.h"

// func sha256(data bytes) bytes {}
TValue _sha256(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    BytesObject *bs = kl_arg_obj_as(0, bytes_type);
    ASSERT(bs != NULL);

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(BYTES_DATA(bs) + BYTES_OFFSET(bs), BYTES_SIZE(bs), digest);
    Object *res = kl_bytes_from_data(digest, SHA256_DIGEST_LENGTH);
    return obj_value(res);
}

/// Entry point called by the runtime after `dlopen("libkoala_crypto.so")`.
///
/// The name must be `<native.name>_native_lib_init`, where
/// `<native.name>` is `crypto` from `__pkg__.toml`.
void crypto_native_lib_init(NativeLib *lib)
{
    printf("Initializing crypto native library\n");
    kl_reg_func(lib, "sha256", _sha256);
}
