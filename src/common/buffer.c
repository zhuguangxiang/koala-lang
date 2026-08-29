/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "buffer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXPAND_MIN_SIZE 4

static int expand(Buffer *self, int min)
{
    int size = self->len + min + 1;
    int newsize = self->size;
    while (newsize <= size) {
        newsize = (newsize > 0) ? (newsize * 2) : EXPAND_MIN_SIZE;
    }

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

static int reserve(Buffer *self, int size)
{
    int left = self->size - self->len - 1;
    if (left <= size && expand(self, size)) return -1;
    return self->size - self->len - 1 - size;
}

int buf_reserve(Buffer *self, size_t size) { return reserve(self, (int)size); }

void buf_write_nstr(Buffer *self, const char *s, int len)
{
    if (!s) return;
    if (len <= 0) return;
    if (reserve(self, len) <= 0) return;
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
    if (reserve(self, 1) <= 0) return;
    self->buf[self->len++] = ch;
}

void buf_write_byte(Buffer *self, uint8_t val)
{
    if (reserve(self, 1) <= 0) return;
    self->buf[self->len++] = val;
}

void buf_write_word(Buffer *self, uint16_t val)
{
    if (reserve(self, 2) <= 0) return;
    uint16_t *ptr = (uint16_t *)(self->buf + self->len);
    *ptr = val;
    self->len += 2;
}

void buf_write_uint8_hex(Buffer *self, uint8_t val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%02x", val);
    buf_write_nstr(self, buf, sz);
}

void buf_write_uint32(Buffer *self, uint32_t val)
{
    if (reserve(self, 4) <= 0) return;
    uint32_t *ptr = (uint32_t *)(self->buf + self->len);
    *ptr = val;
    self->len += 4;
}

void buf_write_int64_str(Buffer *self, int64_t val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%ld", val);
    buf_write_nstr(self, buf, sz);
}

void buf_write_double_str(Buffer *self, double val)
{
    char buf[64];
    int sz = snprintf(buf, 63, "%lf", val);
    buf_write_nstr(self, buf, sz);
}

void escape_str(const char *s, Buffer *buf)
{
    for (const char *p = s; *p; p++) {
        unsigned char c = *p;
        switch (c) {
            case '\n':
                buf_write_str(buf, "\\n");
                break;
            case '\r':
                buf_write_str(buf, "\\r");
                break;
            case '\t':
                buf_write_str(buf, "\\t");
                break;
            case '\\':
                buf_write_str(buf, "\\\\");
                break;
            case '"':
                buf_write_str(buf, "\\\"");
                break;
            case '\'':
                buf_write_str(buf, "\\'");
                break;
            default:
                if (c < 32 || c >= 127) {
                    // control char or non-ASCII → use \xNN
                    char tmp[5];
                    snprintf(tmp, sizeof(tmp), "\\x%02X", c);
                    buf_write_str(buf, tmp);
                } else {
                    buf_write_char(buf, c);
                }
        }
    }
}

/* Stack buffer size for fast path: avoids heap allocation for short formats */
#define BUF_FMT_STACK_SIZE 256

void buf_write_vfmt(Buffer *self, const char *fmt, va_list ap)
{
    if (!self || !fmt) return;

    char stack_buf[BUF_FMT_STACK_SIZE];
    va_list ap_copy;
    va_copy(ap_copy, ap);

    /*
     * First attempt: format into a stack-local buffer.
     * Covers the vast majority of real-world format strings
     * (identifiers, numbers, short messages) without any heap allocation.
     */
    int n = vsnprintf(stack_buf, sizeof(stack_buf), fmt, ap);
    if (n < 0) {
        va_end(ap_copy);
        return; /* encoding error */
    }

    if (n < (int)sizeof(stack_buf)) {
        /* Fast path: result fit in the stack buffer */
        buf_reserve(self, n);
        memcpy(self->buf + self->len, stack_buf, n);
        self->len += n;
    } else {
        /*
         * Slow path: output exceeded stack buffer.
         * 'n' holds the exact number of characters needed (excluding '\0').
         * Re-format directly into the heap buffer.
         */
        buf_reserve(self, n);
        vsnprintf(self->buf + self->len, n + 1, fmt, ap_copy);
        self->len += n;
    }

    va_end(ap_copy);
}

void buf_write_fmt(Buffer *self, const char *fmt, ...)
{
    if (!self || !fmt) return;

    va_list ap;
    va_start(ap, fmt);
    buf_write_vfmt(self, fmt, ap);
    va_end(ap);
}

#ifdef __cplusplus
}
#endif
