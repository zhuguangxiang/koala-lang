/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testhelp.h"
#include "utf8.h"

/* decode `s` (a C string literal, may hold \xNN bytes) and check the result */
#define DEC_IS(s, want_rune, want_w) \
    ({ \
        uint32_t r_ = 0; \
        int w_ = utf8_decode(s, sizeof(s) - 1, &r_); \
        r_ == (uint32_t)(want_rune) && w_ == (want_w); \
    })

#define LAST_IS(s, want_rune, want_w) \
    ({ \
        uint32_t r_ = 0; \
        int w_ = utf8_decode_last(s, sizeof(s) - 1, &r_); \
        r_ == (uint32_t)(want_rune) && w_ == (want_w); \
    })

int utf8Test(void)
{
    uint32_t r = 0;
    char buf[UTF8_MAX_BYTES];

    /* ---------- decode: valid sequences ---------- */
    test_cond("Decode ASCII", DEC_IS("a", 'a', 1));
    test_cond("Decode NUL", DEC_IS("\0", 0, 1));
    test_cond("Decode a 2-byte rune (U+00E9)", DEC_IS("\xc3\xa9", 0xE9, 2));
    test_cond("Decode a 3-byte rune (U+4F60)", DEC_IS("\xe4\xbd\xa0", 0x4F60, 3));
    test_cond("Decode a 4-byte rune (U+1F600)", DEC_IS("\xf0\x9f\x98\x80", 0x1F600, 4));
    test_cond("Decode only the first rune", DEC_IS("\xe4\xbd\xa0\xe5\xa5\xbd", 0x4F60, 3));
    test_cond("Decode U+0080", DEC_IS("\xc2\x80", 0x80, 2));
    test_cond("Decode U+07FF", DEC_IS("\xdf\xbf", 0x7FF, 2));
    test_cond("Decode U+0800", DEC_IS("\xe0\xa0\x80", 0x800, 3));
    test_cond("Decode U+D7FF", DEC_IS("\xed\x9f\xbf", 0xD7FF, 3));
    test_cond("Decode U+E000", DEC_IS("\xee\x80\x80", 0xE000, 3));
    test_cond("Decode U+FFFD as a real rune of width 3", DEC_IS("\xef\xbf\xbd", 0xFFFD, 3));
    test_cond("Decode the noncharacters U+FFFE and U+FFFF",
              DEC_IS("\xef\xbf\xbe", 0xFFFE, 3) && DEC_IS("\xef\xbf\xbf", 0xFFFF, 3));
    test_cond("Decode U+10000", DEC_IS("\xf0\x90\x80\x80", 0x10000, 4));
    test_cond("Decode U+10FFFF", DEC_IS("\xf4\x8f\xbf\xbf", 0x10FFFF, 4));

    /* ---------- decode: empty and invalid input ---------- */
    test_cond("Decode of an empty input has width 0",
              utf8_decode("", 0, &r) == 0 && r == UTF8_RUNE_ERROR);
    test_cond("Decode accepts a NULL rune",
              utf8_decode("a\xc3\xa9", 3, NULL) == 1 && utf8_decode("\xc3\xa9", 2, NULL) == 2);
    test_cond("A lone continuation byte is an error", DEC_IS("\x80", UTF8_RUNE_ERROR, 1));
    test_cond("0xC0 and 0xC1 are never valid leads",
              DEC_IS("\xc0\x80", UTF8_RUNE_ERROR, 1) && DEC_IS("\xc1\xbf", UTF8_RUNE_ERROR, 1));
    test_cond("0xF5..0xFF are never valid leads", DEC_IS("\xf5\x80\x80\x80", UTF8_RUNE_ERROR, 1) &&
                                                      DEC_IS("\xfe", UTF8_RUNE_ERROR, 1) &&
                                                      DEC_IS("\xff", UTF8_RUNE_ERROR, 1));
    test_cond("An overlong 3-byte form is an error", DEC_IS("\xe0\x80\x80", UTF8_RUNE_ERROR, 1));
    test_cond("An overlong 4-byte form is an error",
              DEC_IS("\xf0\x80\x80\x80", UTF8_RUNE_ERROR, 1));
    test_cond("A surrogate (U+D800) is an error", DEC_IS("\xed\xa0\x80", UTF8_RUNE_ERROR, 1));
    test_cond("A surrogate (U+DFFF) is an error", DEC_IS("\xed\xbf\xbf", UTF8_RUNE_ERROR, 1));
    test_cond("Above U+10FFFF is an error", DEC_IS("\xf4\x90\x80\x80", UTF8_RUNE_ERROR, 1));
    test_cond("A truncated 2-byte rune is an error", DEC_IS("\xc3", UTF8_RUNE_ERROR, 1));
    test_cond("A truncated 3-byte rune is an error", DEC_IS("\xe4\xbd", UTF8_RUNE_ERROR, 1));
    test_cond("A truncated 4-byte rune is an error", DEC_IS("\xf0\x9f\x98", UTF8_RUNE_ERROR, 1));
    test_cond("A bad continuation byte is an error",
              DEC_IS("\xc3\x28", UTF8_RUNE_ERROR, 1) && DEC_IS("\xe4\x28\xa0", UTF8_RUNE_ERROR, 1));

    /* ---------- decode_last ---------- */
    test_cond("decode_last of an empty input has width 0",
              utf8_decode_last("", 0, &r) == 0 && r == UTF8_RUNE_ERROR);
    test_cond("decode_last on ASCII", LAST_IS("ab", 'b', 1));
    test_cond("decode_last on a 2-byte rune", LAST_IS("a\xc3\xa9", 0xE9, 2));
    test_cond("decode_last on a 3-byte rune", LAST_IS("a\xe4\xbd\xa0", 0x4F60, 3));
    test_cond("decode_last on a 4-byte rune", LAST_IS("a\xf0\x9f\x98\x80", 0x1F600, 4));
    test_cond("decode_last on a stray continuation byte", LAST_IS("a\x80", UTF8_RUNE_ERROR, 1));
    test_cond("decode_last on a truncated rune", LAST_IS("a\xe4\xbd", UTF8_RUNE_ERROR, 1));
    test_cond("decode_last on a lone lead byte", LAST_IS("a\xe4", UTF8_RUNE_ERROR, 1));
    test_cond("decode_last on a rune followed by a stray byte",
              LAST_IS("\xc3\xa9\x80", UTF8_RUNE_ERROR, 1));
    test_cond("decode_last on five continuation bytes",
              LAST_IS("\x80\x80\x80\x80\x80", UTF8_RUNE_ERROR, 1));
    test_cond("decode_last accepts a NULL rune", utf8_decode_last("a\xc3\xa9", 3, NULL) == 2);

    /* ---------- encode ---------- */
    test_cond("Encode ASCII", utf8_encode('a', buf) == 1 && buf[0] == 'a');
    test_cond("Encode a 2-byte rune",
              utf8_encode(0xE9, buf) == 2 && memcmp(buf, "\xc3\xa9", 2) == 0);
    test_cond("Encode a 3-byte rune",
              utf8_encode(0x4F60, buf) == 3 && memcmp(buf, "\xe4\xbd\xa0", 3) == 0);
    test_cond("Encode a 4-byte rune",
              utf8_encode(0x1F600, buf) == 4 && memcmp(buf, "\xf0\x9f\x98\x80", 4) == 0);
    test_cond("Encode a surrogate gives U+FFFD",
              utf8_encode(0xD800, buf) == 3 && memcmp(buf, "\xef\xbf\xbd", 3) == 0);
    test_cond("Encode above U+10FFFF gives U+FFFD",
              utf8_encode(0x110000, buf) == 3 && memcmp(buf, "\xef\xbf\xbd", 3) == 0);

    /* ---------- rune helpers ---------- */
    test_cond("valid_rune bounds", utf8_valid_rune(0) && utf8_valid_rune(0xD7FF) &&
                                       !utf8_valid_rune(0xD800) && !utf8_valid_rune(0xDFFF) &&
                                       utf8_valid_rune(0xE000) && utf8_valid_rune(0x10FFFF) &&
                                       !utf8_valid_rune(0x110000));
    test_cond("rune_len", utf8_rune_len(0x7F) == 1 && utf8_rune_len(0x80) == 2 &&
                              utf8_rune_len(0x7FF) == 2 && utf8_rune_len(0x800) == 3 &&
                              utf8_rune_len(0xFFFF) == 3 && utf8_rune_len(0x10000) == 4 &&
                              utf8_rune_len(0x10FFFF) == 4 && utf8_rune_len(0xD800) == -1 &&
                              utf8_rune_len(0x110000) == -1);

    /* ---------- every valid rune round-trips ---------- */
    int ok = 1;
    for (uint32_t c = 0; c <= UTF8_MAX_RUNE; c++) {
        if (!utf8_valid_rune(c)) continue;

        int w = utf8_encode(c, buf);
        uint32_t back = 0, last = 0;
        if (w != utf8_rune_len(c) || utf8_decode(buf, (size_t)w, &back) != w || back != c ||
            utf8_decode_last(buf, (size_t)w, &last) != w || last != c) {
            ok = 0;
            break;
        }
    }
    test_cond("Every valid rune round-trips through encode, decode and decode_last", ok);

    /* ---------- validity and counting ---------- */
    test_cond("valid: empty and ASCII", utf8_valid("", 0) && utf8_valid("koala", 5));
    test_cond("valid: multi-byte text", utf8_valid("\xe4\xbd\xa0\xe5\xa5\xbd", 6));
    test_cond("valid: noncharacters are accepted", utf8_valid("\xef\xbf\xbe\xef\xbf\xbf", 6));
    test_cond("valid: U+FFFD itself is valid", utf8_valid("\xef\xbf\xbd", 3));
    test_cond("valid: rejects a stray byte", !utf8_valid("a\x80", 2));
    test_cond("valid: rejects a truncated rune", !utf8_valid("a\xe4\xbd", 3));
    test_cond("valid: rejects a surrogate", !utf8_valid("\xed\xa0\x80", 3));

    test_cond("rune_count: empty and ASCII",
              utf8_rune_count("", 0) == 0 && utf8_rune_count("koala", 5) == 5);
    test_cond("rune_count: multi-byte text",
              utf8_rune_count("a\xe4\xbd\xa0\xf0\x9f\x98\x80", 8) == 3);
    test_cond("rune_count: each invalid byte counts as one rune", utf8_rune_count("\x80\x80"
                                                                                  "a\xe4\xbd",
                                                                                  5) == 5);
    test_cond("rune_count: counts an embedded NUL", utf8_rune_count("a\0b", 3) == 3);

    /* ---------- check_utf8 keeps its stricter contract ---------- */
    test_cond("check_utf8 still counts characters",
              check_utf8("\xe4\xbd\xa0\xe5\xa5\xbd", 6) == 2 && check_utf8("a\x80", 2) == -1);
    test_cond("check_utf8 still rejects the noncharacters",
              check_utf8("\xef\xbf\xbe", 3) == -1 && utf8_valid("\xef\xbf\xbe", 3));

    test_report();
    return 0;
}

int main(void) { return utf8Test(); }
