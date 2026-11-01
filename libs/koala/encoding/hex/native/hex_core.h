/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * RFC 4648 section 8 hexadecimal codec. No Koala runtime dependencies, so
 * the algorithm can be unit-tested (and fuzzed against CPython) on its own.
 * Laid out like base64_core.h: the shim above it only translates TValue
 * arguments and results, never touches the byte loop.
 */

#ifndef _KOALA_HEX_CORE_H_
#define _KOALA_HEX_CORE_H_

#include <stddef.h>
#include <stdint.h>

/*---------------------------------------------------------------------------+
 |   Error codes                                                             |
 +---------------------------------------------------------------------------*/

/* Returned by hex_decode in place of a length. */
#define HEX_OK             0
#define HEX_ERR_DIGIT    (-1)   /* byte outside 0-9, a-f, A-F */
#define HEX_ERR_ODD      (-2)   /* odd number of characters */

/*---------------------------------------------------------------------------+
 |   Encode                                                                  |
 +---------------------------------------------------------------------------*/

/* The 16-byte digit table, lowercase when upper is 0, uppercase otherwise.
 * Never returns NULL. */
const char *hex_digits(int upper);

/* Chars written by hex_encode for n input bytes. */
size_t hex_encoded_len(size_t n);

/* Write exactly 2 * n chars. digits must come from hex_digits(). */
void hex_encode(const uint8_t *in, size_t n, uint8_t *out, const char *digits);

/*---------------------------------------------------------------------------+
 |   Decode                                                                  |
 +---------------------------------------------------------------------------*/

/* Decode n chars. Both cases are accepted and nothing else is.
 *
 * A bad byte is reported before an odd length, so "z" yields HEX_ERR_ODD
 * while "zz" yields HEX_ERR_DIGIT. That is the order the pure Koala
 * version used, and the panic message of each test depends on it. */
ptrdiff_t hex_decode(const uint8_t *in, size_t n, uint8_t *out);

#endif /* _KOALA_HEX_CORE_H_ */