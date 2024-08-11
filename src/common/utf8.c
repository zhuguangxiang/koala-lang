/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "common.h"
#include "log.h"

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

#ifdef __cplusplus
}
#endif
