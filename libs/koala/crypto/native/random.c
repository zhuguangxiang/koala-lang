/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Cryptographically secure randomness, matching the @native
 * declarations in random.kl. token_hex and token_urlsafe are Koala
 * code built on random_bytes and live in random.kl, not here.
 */

#include <openssl/rand.h>
#include <stddef.h>
#include <stdint.h>
#include "bytesobj.h"

// func random_bytes(n int) bytes
static TValue _random_bytes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t n = kl_arg_int(0);
    ASSERT(n >= 0);

    Object *res = kl_new_bytes((size_t)n);
    if (n > 0) {
        BytesObject *bs = (BytesObject *)res;
        int rc = RAND_bytes(BYTES_DATA(bs) + BYTES_OFFSET(bs), (int)n);
        ASSERT(rc == 1);
    }
    return obj_value(res);
}

// func randbelow(n int) int
//
// Rejection sampling to avoid modulo bias: draw ceil(log2(n)) bits
// until the result is < n. Matches Python's secrets.randbelow.
static TValue _randbelow(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t n = kl_arg_int(0);
    ASSERT(n > 0);
    if (n == 1) {
        return int64_value(0);
    }

    uint64_t limit = (uint64_t)(n - 1);
    int bits = 0;
    while (limit > 0) {
        bits += 1;
        limit >>= 1;
    }
    int bytes = (bits + 7) / 8;
    uint64_t mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1ULL);

    uint64_t r = 0;
    do {
        unsigned char buf[8];
        RAND_bytes(buf, bytes);
        r = 0;
        for (int i = 0; i < bytes; i++) {
            r = (r << 8) | buf[i];
        }
        r &= mask;
    } while (r >= (uint64_t)n);
    return int64_value((int64_t)r);
}

// func randbits(k int) int
//
// A uniform integer in [0, 2^k). The top bit is not forced, matching
// Python's secrets.randbits semantics.
static TValue _randbits(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t k = kl_arg_int(0);
    ASSERT(k > 0);
    if (k > 63) {
        k = 63;
    }

    int bytes = (int)((k + 7) / 8);
    unsigned char buf[8];
    RAND_bytes(buf, bytes);

    uint64_t r = 0;
    for (int i = 0; i < bytes; i++) {
        r = (r << 8) | buf[i];
    }
    r &= ((1ULL << k) - 1ULL);
    return int64_value((int64_t)r);
}

void crypto_random_register(NativeLib *lib)
{
    kl_reg_func(lib, "random_bytes", _random_bytes);
    kl_reg_func(lib, "randbelow", _randbelow);
    kl_reg_func(lib, "randbits", _randbits);
}
