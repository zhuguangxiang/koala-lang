/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
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

/* A Dynamic Buffer. */

#ifndef _KOALA_BUF_H_
#define _KOALA_BUF_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* buffer */
typedef struct _Buffer {
    /* allocated size */
    int size;
    /* used size */
    int len;
    /* contains string */
    char *buf;
} Buffer, *BufferRef;

/* Declare a buffer. */
#define BUF(name) Buffer name = { 0, 0, NULL }

/* Free a buffer. */
#define FINI_BUF(name) MemFree((name).buf)

// clang-format off
/* Reset a buffer. */
#define RESET_BUF(name) do { \
    memset((name).buf, 0, (name).len); (name).len = 0; \
} while (0)
// clang-format on

/* Get null-terminated string from the buffer. */
#define BUF_STR(name) (name).buf

/* Get the buffer length */
#define BUF_LEN(name) (name).len

/* Write string with len */
void BufWriteNStr(BufferRef self, char *s, int len);

/* Write a null-terminated string. */
void BufWriteStr(BufferRef self, char *s);

/* Write 'num' null-terminated strings. */
void BufNWrite(BufferRef self, int num, ...);
void BufVWrite(BufferRef self, int num, va_list args);

void BufWriteChar(BufferRef self, char ch);
void BufWriteByte(BufferRef self, int val);
void BufWriteInt(BufferRef self, int ch);
void BufWriteInt64(BufferRef self, int64_t val);
void BufWriteDouble(BufferRef self, double val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BUF_H_ */
