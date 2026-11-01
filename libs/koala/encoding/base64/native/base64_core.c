/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * RFC 4648 base64 codec. Nothing here knows about TValue, bytes objects or
 * the exception machinery; base64_shim.c owns that translation.
 */

#include "base64_core.h"

#include <string.h>

/* The 64 alphabet bytes, indexed by 6-bit value. */
static const uint8_t STD_TABLE[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* byte -> 6-bit value, or -1 for anything outside the alphabet. Filled by
 * b64_core_init(), which rev_ready guards. */
static int8_t rev_table[256];
static int rev_ready = 0;

void b64_core_init(void)
{
    if (rev_ready) return;

    memset(rev_table, -1, sizeof rev_table);
    for (int i = 0; i < 64; i++) {
        rev_table[STD_TABLE[i]] = (int8_t)i;
    }

    rev_ready = 1;
}

size_t b64_encoded_len(size_t n)
{
    return ((n + 2) / 3) * 4;
}

size_t b64_decoded_max_len(size_t n)
{
    return (n / 4) * 3 + 3;
}

size_t b64_encodebytes_len(size_t n)
{
    size_t enc, lines;
    if (n == 0) return 0;
    enc = b64_encoded_len(n);
    lines = (enc + 75) / 76;
    return enc + lines;
}

void b64_encode(const uint8_t *in, size_t n, uint8_t *out, const uint8_t *alt)
{
    uint8_t tbl[64];
    size_t i = 0, o = 0;

    /* Copy the standard alphabet, then patch the two altchars slots. */
    memcpy(tbl, STD_TABLE, 64);
    if (alt) {
        tbl[62] = alt[0];
        tbl[63] = alt[1];
    }

    /* Whole 3-byte groups: 24 bits in, four 6-bit indices out. */
    for (; i + 2 < n; i += 3) {
        uint32_t v = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8) | in[i + 2];
        out[o++] = tbl[(v >> 18) & 63];
        out[o++] = tbl[(v >> 12) & 63];
        out[o++] = tbl[(v >> 6) & 63];
        out[o++] = tbl[v & 63];
    }

    /* Trailing 1 or 2 bytes, padded with '=' up to a full four chars. */
    if (i < n) {
        size_t rem = n - i;
        uint32_t v = (uint32_t)in[i] << 16;

        if (rem == 2) v |= (uint32_t)in[i + 1] << 8;

        out[o++] = tbl[(v >> 18) & 63];
        out[o++] = tbl[(v >> 12) & 63];
        if (rem == 2) {
            out[o++] = tbl[(v >> 6) & 63];
            out[o++] = '=';
        } else {
            out[o++] = '=';
            out[o++] = '=';
        }
    }
}

/* Count the '=' that sit at the very end of the input, stepping over bytes
 * outside the alphabet the way lax mode does. */
static size_t _trailing_padding(const uint8_t *in, size_t n, const int8_t *lookup)
{
    size_t tail_eq = 0;

    for (size_t i = n; i > 0; i--) {
        int c = in[i - 1];

        if (c == '=') {
            tail_eq++;
            continue;
        }

        /* A skipped byte is not part of the tail: keep looking past it. */
        if (lookup[c] < 0) continue;

        break;
    }

    return tail_eq;
}

ptrdiff_t b64_decode(const uint8_t *in, size_t n, uint8_t *out,
                     const uint8_t *alt, int strict)
{
    int8_t rev[256];
    size_t data = 0, eq_total = 0, need, tail_eq;
    uint32_t acc = 0;
    int bits = 0;
    uint8_t *o = out;
    const int8_t *lookup;

    b64_core_init();

    /* Pick the reverse table: the shared one, or a copy patched for alt. */
    if (alt) {
        memcpy(rev, rev_table, sizeof rev);
        rev[alt[0]] = 62;
        rev[alt[1]] = 63;
        lookup = rev;
    } else {
        lookup = rev_table;
    }

    /* Every input byte goes through the same pipeline: '=' is counted,
     * alphabet bytes feed the 6-bit accumulator, and (in lax mode)
     * anything else is skipped, which is what CPython does. */
    for (size_t i = 0; i < n; i++) {
        int c = in[i];
        int8_t v;

        if (c == '=') {
            eq_total++;
            continue;
        }

        v = lookup[c];
        if (v < 0) {
            if (strict) return B64_ERR_CHAR;
            continue;
        }

        data++;
        acc = (acc << 6) | (uint32_t)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            *o++ = (uint8_t)((acc >> bits) & 0xFF);
        }
    }

    if (data % 4 == 1) return B64_ERR_INVALID;

    /* %4==2 needs two '=', %4==3 needs one, %4==0 needs none. */
    need = (4 - data % 4) % 4;
    if (need == 3) need = 1;

    tail_eq = _trailing_padding(in, n, lookup);
    if (tail_eq < need) return B64_ERR_PADDING;

    /* Strict mode wants the padding to sit at the very end and to be
     * exactly as long as the data demands. */
    if (strict) {
        if (eq_total != tail_eq) return B64_ERR_EXCESS;
        if (tail_eq > need) return B64_ERR_EXCESS;
    }

    return (ptrdiff_t)(o - out);
}

ptrdiff_t b64_encodebytes(const uint8_t *in, size_t n, uint8_t *out,
                          const uint8_t *alt)
{
    uint8_t line[76];
    size_t i = 0, o = 0;

    /* 57 input bytes -> 76 output chars, so one line per 57-byte chunk.
     * A trailing newline follows every line, including the last. */
    while (i < n) {
        size_t chunk = (n - i < 57) ? (n - i) : 57;
        size_t clen = b64_encoded_len(chunk);

        b64_encode(in + i, chunk, line, alt);
        memcpy(out + o, line, clen);
        o += clen;
        out[o++] = '\n';
        i += chunk;
    }

    return (ptrdiff_t)o;
}
