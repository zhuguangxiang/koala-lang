/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_UTF8_H_
#define _KOALA_UTF8_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// return count of utf8 characters, or -1 if not valid.
int check_utf8(void *str, int len);

/*---------------------------------------------------------------------------+
 |   Rune API                                                                |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |   Code point helpers for C code that has to look inside a `str`. Koala    |
 |   strings are raw bytes, and the `str` methods are byte based, so these   |
 |   are not exposed to Koala code.                                          |
 |                                                                           |
 |   The semantics follow Go's unicode/utf8, not check_utf8():               |
 |                                                                           |
 |   - an invalid byte decodes as U+FFFD with a width of 1, so scanning     |
 |     always makes progress;                                                |
 |   - overlong forms, surrogates (U+D800..U+DFFF) and values above         |
 |     U+10FFFF are invalid;                                                 |
 |   - the noncharacters U+FFFE and U+FFFF are valid, check_utf8() rejects   |
 |     them.                                                                 |
 |                                                                           |
 |   The two one-liners are `static inline`; the rest lives in utf8.c.       |
 |   Native packages are shared objects and resolve them from libkoala.so,   |
 |   which compiles utf8.c in (see KOALA_SHARED_SOURCES).                    |
 +---------------------------------------------------------------------------*/

#define UTF8_RUNE_ERROR 0xFFFD
#define UTF8_MAX_RUNE   0x10FFFF
#define UTF8_MAX_BYTES  4

/* 1 if `r` is a Unicode scalar value (not a surrogate, not above U+10FFFF). */
static inline int utf8_valid_rune(uint32_t r)
{
    return r < 0xD800 || (r > 0xDFFF && r <= UTF8_MAX_RUNE);
}

/* Bytes needed to encode `r`, or -1 if it is not a valid rune. */
static inline int utf8_rune_len(uint32_t r)
{
    if (r < 0x80) return 1;
    if (r < 0x800) return 2;
    if (!utf8_valid_rune(r)) return -1;
    return r < 0x10000 ? 3 : 4;
}

/*
 * Decode the rune at the start of s[0, n) into *rune and return its width.
 *
 * - n == 0: *rune = U+FFFD, returns 0.
 * - invalid or truncated sequence: *rune = U+FFFD, returns 1.
 *
 * A real U+FFFD in the input is told apart from an error by its width of 3.
 * `rune` may be NULL when only the width is wanted.
 */
int utf8_decode(const char *s, size_t n, uint32_t *rune);

/*
 * Decode the last rune of s[0, n) into *rune and return its width, with the
 * same rules as utf8_decode(): n == 0 gives 0, an invalid tail gives
 * U+FFFD and a width of 1.
 */
int utf8_decode_last(const char *s, size_t n, uint32_t *rune);

/*
 * Write the UTF-8 form of `r` to out, which must have room for
 * UTF8_MAX_BYTES, and return the width. An invalid rune is written as
 * U+FFFD.
 */
int utf8_encode(uint32_t r, char *out);

/* 1 if s[0, n) is entirely valid UTF-8 (Go rules), else 0. */
int utf8_valid(const char *s, size_t n);

/* Number of runes in s[0, n); every invalid byte counts as one rune. */
size_t utf8_rune_count(const char *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_UTF8_H_ */
