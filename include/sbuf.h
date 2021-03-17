/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* dynamic string buffer. */

#ifndef _KOALA_SBUF_H_
#define _KOALA_SBUF_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* string buffer */
typedef struct _SBuf {
    /* allocated size */
    int size;
    /* used size */
    int len;
    /* contains string */
    char *buf;
} SBuf, *SBufRef;

/* Declare a string buffer. */
#define SBUF(name) SBuf name = { 0, 0, NULL }

/* Free a string buffer. */
#define FINI_SBUF(name) mm_free((name).buf)

// clang-format off
/* Reset a string buffer. */
#define RESET_SBUF(name) do { \
    memset((name).buf, 0, (name).len); (name).len = 0; \
} while (0)
// clang-format on

/* Get null-terminated string from the string buffer. */
#define SBUF_STR(name) (name).buf

/* Get string length */
#define SBUF_LEN(name) (name).len

/* Write string with len */
void sbuf_nprint(SBufRef self, char *s, int len);

/* Write a null-terminated string. */
void sbuf_print(SBufRef self, char *s);

/* Write 'num' null-terminated strings. */
void sbuf_nsprint(SBufRef self, int num, ...);
void sbuf_vnprint(SBufRef self, int num, va_list args);

void sbuf_print_char(SBufRef self, char ch);
void sbuf_print_byte(SBufRef self, int val);
void sbuf_print_int(SBufRef self, int ch);
void sbuf_print_int64(SBufRef self, int64_t val);
void sbuf_print_double(SBufRef self, double val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SBUF_H_ */
