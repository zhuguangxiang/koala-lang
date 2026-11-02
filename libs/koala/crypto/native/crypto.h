/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CRYPTO_H_
#define _KOALA_CRYPTO_H_

#include "object.h"

// Each source file in native/ owns one Koala file's @native
// functions and exposes a single register entry. crypto_native_lib_init
// in crypto_shim.c calls them all.

void crypto_hash_register(NativeLib *lib);
void crypto_hmac_register(NativeLib *lib);
void crypto_random_register(NativeLib *lib);

#endif /* _KOALA_CRYPTO_H_ */
