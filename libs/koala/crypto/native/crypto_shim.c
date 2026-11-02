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

#include "crypto.h"
#include "object.h"

/// Entry point called by the runtime after `dlopen("libkoala_crypto.so")`.
///
/// The name must be `<native.name>_native_lib_init`, where
/// `<native.name>` is `crypto` from `__pkg__.toml`.
void crypto_native_lib_init(NativeLib *lib)
{
    crypto_hash_register(lib);
    crypto_hmac_register(lib);
    crypto_random_register(lib);
}
