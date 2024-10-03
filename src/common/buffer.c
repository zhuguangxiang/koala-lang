/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXPAND_MIN_SIZE 4

static int expand(Buffer *self, int min)
{
    int size = self->len + min + 1;
    int newsize = self->size;
    while (newsize <= size) newsize += EXPAND_MIN_SIZE;

    char *newbuf = mm_alloc(newsize);
    if (!newbuf) return -1;

    if (self->buf) {
        memcpy(newbuf, self->buf, self->len);
        mm_free(self->buf);
    }
    self->buf = newbuf;
    self->size = newsize;
    return 0;
}

static int available(Buffer *self, int size)
{
    int left = self->size - self->len - 1;
    if (left <= size && expand(self, size)) return -1;
    return self->size - self->len - 1 - size;
}

void buf_write_nstr(Buffer *self, const char *s, int len)
{
    if (!s) return;
    if (len <= 0) return;
    if (available(self, len) <= 0) return;
    strncat(self->buf, s, len);
    self->len += len;
}

void buf_write_str(Buffer *self, const char *s)
{
    if (!s) return;
    int len = strlen(s);
    return buf_write_nstr(self, s, len);
}

void buf_nwrite(Buffer *self, int count, ...)
{
    char *s;
    va_list args;
    va_start(args, count);
    while (count-- > 0) {
        s = va_arg(args, char *);
        buf_write_str(self, s);
    }
    va_end(args);
}

void buf_write_char(Buffer *self, char ch)
{
    if (available(self, 1) <= 0) return;
    self->buf[self->len++] = ch;
}

void buf_write_int(Buffer *self, int ch)
{
    char buf[64];
    if (ch < 255) {
        snprintf(buf, 63, "'%c'", (char)ch);
        buf_write_nstr(self, buf, 3);
    } else {
        buf_write_char(self, '\'');
        buf_write_str(self, (char *)&ch);
        buf_write_char(self, '\'');
    }
}

void buf_write_byte(Buffer *self, int val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%d", val);
    buf_write_nstr(self, buf, sz);
}

void buf_write_int64(Buffer *self, int64_t val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%ld", val);
    buf_write_nstr(self, buf, sz);
}

void buf_write_double(Buffer *self, double val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%lf", val);
    buf_write_nstr(self, buf, sz);
}

#ifdef __cplusplus
}
#endif
