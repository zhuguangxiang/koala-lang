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

#include "sbuf.h"

#define EXPAND_MIN_SIZE 32

static int expand(SBufRef self, int min)
{
    int size = self->len + min + 1;
    int newsize = self->size;
    while (newsize <= size) newsize += EXPAND_MIN_SIZE;

    char *newbuf = mm_alloc(newsize);
    if (newbuf == NULL) return -1;

    if (self->buf != NULL) {
        strcpy(newbuf, self->buf);
        mm_free(self->buf);
    }
    self->buf = newbuf;
    self->size = newsize;
    return 0;
}

static int available(SBufRef self, int size)
{
    int left = self->size - self->len - 1;
    if (left <= size && expand(self, size)) return -1;
    return self->size - self->len - 1 - size;
}

void sbuf_nprint(SBufRef self, char *s, int len)
{
    if (s == NULL) return;
    if (len <= 0) return;
    if (available(self, len) <= 0) return;
    strncat(self->buf, s, len);
    self->len += len;
}

void sbuf_print(SBufRef self, char *s)
{
    if (s == NULL) return;
    int len = strlen(s);
    sbuf_nprint(self, s, len);
}

void sbuf_vnprint(SBufRef self, int count, va_list args)
{
    char *s;
    while (count-- > 0) {
        s = va_arg(args, char *);
        sbuf_print(self, s);
    }
}

void sbuf_nsprint(SBufRef self, int count, ...)
{
    va_list args;
    va_start(args, count);
    sbuf_vnprint(self, count, args);
    va_end(args);
}

void sbuf_print_char(SBufRef self, char ch)
{
    if (available(self, 1) <= 0) return;
    self->buf[self->len++] = ch;
}

void sbuf_print_int(SBufRef self, int ch)
{
    char buf[64];
    if (ch < 255) {
        snprintf(buf, 63, "'%c'", (char)ch);
        sbuf_nprint(self, buf, 3);
    } else {
        sbuf_print_char(self, '\'');
        sbuf_print(self, (char *)&ch);
        sbuf_print_char(self, '\'');
    }
}

void sbuf_print_byte(SBufRef self, int val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%d", val);
    sbuf_nprint(self, buf, sz);
}

void sbuf_print_int64(SBufRef self, int64_t val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%ld", val);
    sbuf_nprint(self, buf, sz);
}

void sbuf_print_double(SBufRef self, double val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%lf", val);
    sbuf_nprint(self, buf, sz);
}
