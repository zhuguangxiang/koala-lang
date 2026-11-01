/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

int check_utf8(void *str, int len)
{
    unsigned char *s = (unsigned char *)str;
    unsigned char *end = s + len;
    int count = 0;

    while (s < end) {
        if (*s < 0x80) {
            /* 0xxx_xxxx */
            s += 1;
            ++count;
        } else if ((s[0] & 0xe0) == 0xc0) {
            /* 110X_XXXx 10xx_xxxx */
            if ((s[1] & 0xc0) != 0x80 || (s[0] & 0xfe) == 0xc0) return -1;
            s += 2;
            ++count;
        } else if ((s[0] & 0xf0) == 0xe0) {
            /* 1110_XXXX 10Xx_xxxx 10xx_xxxx */
            if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 ||
                /* overlong? */
                (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||
                /* surrogate? */
                (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||
                /* U+FFFE or U+FFFF? */
                (s[0] == 0xef && s[1] == 0xbf && (s[2] & 0xfe) == 0xbe))
                return -1;
            s += 3;
            ++count;
        } else if ((s[0] & 0xf8) == 0xf0) {
            /* 1111_0XXX 10XX_xxxx 10xx_xxxx 10xx_xxxx */
            if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80 ||
                /* overlong? */
                (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||
                /* > U+10FFFF? */
                (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4)
                return -1;
            s += 4;
            ++count;
        } else {
            return -1;
        }
    }

    return count;
}

/*---------------------------------------------------------------------------+
 |   Rune API, see utf8.h                                                    |
 +---------------------------------------------------------------------------*/

int utf8_decode(const char *s, size_t n, uint32_t *rune)
{
    const unsigned char *p = (const unsigned char *)s;
    uint32_t r = UTF8_RUNE_ERROR;
    int w = 1;

    if (n == 0) {
        w = 0;
    } else if (p[0] < 0x80) {
        r = p[0];
    } else if (p[0] >= 0xC2 && p[0] <= 0xDF) {
        if (n >= 2 && (p[1] & 0xC0) == 0x80) {
            r = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            w = 2;
        }
    } else if (p[0] >= 0xE0 && p[0] <= 0xEF) {
        /* E0 needs A0..BF (no overlong), ED needs 80..9F (no surrogate) */
        unsigned char lo = p[0] == 0xE0 ? 0xA0 : 0x80;
        unsigned char hi = p[0] == 0xED ? 0x9F : 0xBF;

        if (n >= 3 && p[1] >= lo && p[1] <= hi && (p[2] & 0xC0) == 0x80) {
            r = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            w = 3;
        }
    } else if (p[0] >= 0xF0 && p[0] <= 0xF4) {
        /* F0 needs 90..BF (no overlong), F4 needs 80..8F (<= U+10FFFF) */
        unsigned char lo = p[0] == 0xF0 ? 0x90 : 0x80;
        unsigned char hi = p[0] == 0xF4 ? 0x8F : 0xBF;

        if (n >= 4 && p[1] >= lo && p[1] <= hi && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
            r = ((uint32_t)(p[0] & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12) |
                ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
            w = 4;
        }
    }

    if (rune) *rune = r;
    return w;
}

int utf8_decode_last(const char *s, size_t n, uint32_t *rune)
{
    size_t start, lim;
    int w;

    if (n == 0) {
        if (rune) *rune = UTF8_RUNE_ERROR;
        return 0;
    }

    /* A rune is at most 4 bytes: look back over continuation bytes only. */
    lim = n >= UTF8_MAX_BYTES ? n - UTF8_MAX_BYTES : 0;
    start = n - 1;
    while (start > lim && ((unsigned char)s[start] & 0xC0) == 0x80) start--;

    w = utf8_decode(s + start, n - start, rune);
    if (start + (size_t)w != n) {
        /* the candidate does not end at n, so the last byte is stray */
        if (rune) *rune = UTF8_RUNE_ERROR;
        return 1;
    }

    return w;
}

int utf8_encode(uint32_t r, char *out)
{
    unsigned char *o = (unsigned char *)out;

    if (r < 0x80) {
        o[0] = (unsigned char)r;
        return 1;
    }
    if (r < 0x800) {
        o[0] = (unsigned char)(0xC0 | (r >> 6));
        o[1] = (unsigned char)(0x80 | (r & 0x3F));
        return 2;
    }
    if (!utf8_valid_rune(r)) r = UTF8_RUNE_ERROR;
    if (r < 0x10000) {
        o[0] = (unsigned char)(0xE0 | (r >> 12));
        o[1] = (unsigned char)(0x80 | ((r >> 6) & 0x3F));
        o[2] = (unsigned char)(0x80 | (r & 0x3F));
        return 3;
    }
    o[0] = (unsigned char)(0xF0 | (r >> 18));
    o[1] = (unsigned char)(0x80 | ((r >> 12) & 0x3F));
    o[2] = (unsigned char)(0x80 | ((r >> 6) & 0x3F));
    o[3] = (unsigned char)(0x80 | (r & 0x3F));
    return 4;
}

int utf8_valid(const char *s, size_t n)
{
    size_t i = 0;

    while (i < n) {
        uint32_t r;
        int w = utf8_decode(s + i, n - i, &r);

        if (w == 1 && r == UTF8_RUNE_ERROR) return 0;
        i += (size_t)w;
    }

    return 1;
}

size_t utf8_rune_count(const char *s, size_t n)
{
    size_t i = 0, count = 0;

    while (i < n) {
        if ((unsigned char)s[i] < 0x80) {
            i++;
        } else {
            i += (size_t)utf8_decode(s + i, n - i, NULL);
        }
        count++;
    }

    return count;
}

#ifdef __cplusplus
}
#endif
