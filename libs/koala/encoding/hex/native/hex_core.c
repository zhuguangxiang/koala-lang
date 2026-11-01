/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * RFC 4648 section 8 hexadecimal codec. Nothing here knows about TValue,
 * bytes objects or the exception machinery; hex_shim.c owns that
 * translation.
 */

#include "hex_core.h"

/* The two alphabets, indexed by the 4-bit value. */
static const char DIGITS_LOWER[17] = "0123456789abcdef";
static const char DIGITS_UPPER[17] = "0123456789ABCDEF";

const char *hex_digits(int upper)
{
    return upper ? DIGITS_UPPER : DIGITS_LOWER;
}

size_t hex_encoded_len(size_t n)
{
    return n * 2;
}

void hex_encode(const uint8_t *in, size_t n, uint8_t *out, const char *digits)
{
    size_t i = 0, o = 0;

    /* One byte in, two digits out: the high nibble comes first. */
    for (; i < n; i++) {
        uint8_t b = in[i];

        out[o++] = (uint8_t)digits[b >> 4];
        out[o++] = (uint8_t)digits[b & 0x0f];
    }
}

/* ASCII -> 4-bit value, or -1 for anything that is not a hex digit. */
static int _digit_value(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    return -1;
}

ptrdiff_t hex_decode(const uint8_t *in, size_t n, uint8_t *out)
{
    size_t i = 0, o = 0;

    /* Walk the input in pairs: a bad byte fails here, in either half. */
    while (i + 1 < n) {
        int hi = _digit_value(in[i]);
        int lo = _digit_value(in[i + 1]);

        if (hi < 0 || lo < 0) return HEX_ERR_DIGIT;

        out[o++] = (uint8_t)(hi * 16 + lo);
        i += 2;
    }

    /* Only a leftover final character means a bad length, so the single
     * char "z" is reported as odd rather than as a bad digit. That is the
     * order the pure Koala version used, and the panic message of each
     * test depends on it. */
    if (i != n) return HEX_ERR_ODD;

    return (ptrdiff_t)o;
}