/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * RFC 4648 base64 codec. No Koala runtime dependencies, so the
 * algorithm can be unit-tested (and fuzzed against CPython) on its own.
 */

#ifndef _KOALA_BASE64_CORE_H_
#define _KOALA_BASE64_CORE_H_

#include <stddef.h>
#include <stdint.h>

/*---------------------------------------------------------------------------+
 |   Error codes                                                             |
 +---------------------------------------------------------------------------*/

/* Returned by b64_decode / b64_encodebytes in place of a length. */
#define B64_OK             0
#define B64_ERR_INVALID   (-1)   /* data char count is 1 more than a multiple of 4 */
#define B64_ERR_PADDING   (-2)   /* missing or misplaced padding */
#define B64_ERR_EXCESS    (-3)   /* more padding than the data needs (strict) */
#define B64_ERR_CHAR      (-4)   /* non-alphabet byte (strict) */

/*---------------------------------------------------------------------------+
 |   Life cycle                                                              |
 +---------------------------------------------------------------------------*/

/* Build the reverse lookup table. Idempotent; every entry point calls it,
 * so code outside the runtime does not have to. */
void b64_core_init(void);

/*---------------------------------------------------------------------------+
 |   Output length                                                           |
 +---------------------------------------------------------------------------*/

/* Chars written by b64_encode for n input bytes, padding included. */
size_t b64_encoded_len(size_t n);

/* Upper bound on the bytes written by b64_decode for n input chars: every
 * complete 4-char group yields 3 bytes, the trailing partial one at most 3,
 * hence the "+ 3". */
size_t b64_decoded_max_len(size_t n);

/* Bytes written by b64_encodebytes for n input bytes, newlines included. */
size_t b64_encodebytes_len(size_t n);

/*---------------------------------------------------------------------------+
 |   Encode and decode                                                       |
 +---------------------------------------------------------------------------*/

/* alt: NULL, or two bytes replacing '+' and '/'. */
void b64_encode(const uint8_t *in, size_t n, uint8_t *out, const uint8_t *alt);

/* strict != 0 rejects bytes outside the alphabet and excess padding. */
ptrdiff_t b64_decode(const uint8_t *in, size_t n, uint8_t *out,
                     const uint8_t *alt, int strict);

/* RFC 2045 style: 76 columns, trailing newline after the last line. */
ptrdiff_t b64_encodebytes(const uint8_t *in, size_t n, uint8_t *out,
                          const uint8_t *alt);

#endif /* _KOALA_BASE64_CORE_H_ */
