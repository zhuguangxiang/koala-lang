/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include <stdio.h>
#include "strbuf.h"

#define EXPAND_MIN_SIZE 32

static int expand(StrBuf *self, int min)
{
  int size = self->len + min + 1;
  int newsize = self->size;
  while (newsize <= size)
    newsize += EXPAND_MIN_SIZE;

  char *newbuf = kmalloc(newsize);
  if (newbuf == NULL)
    return -1;

  if (self->buf != NULL) {
    strcpy(newbuf, self->buf);
    kfree(self->buf);
  }
  self->buf  = newbuf;
  self->size = newsize;
  return 0;
}

static int available(StrBuf *self, int size)
{
  int left = self->size - self->len - 1;
  if (left <= size && expand(self, size))
    return -1;
  return self->size - self->len - 1 - size;
}

void strbuf_nappend(StrBuf *self, char *s, int len)
{
  if (s == NULL)
    return;

  if (len <= 0)
    return;

  if (available(self, len) <= 0)
    return;

  strncat(self->buf, s, len);
  self->len += len;
}

void strbuf_append(StrBuf *self, char *s)
{
  if (s == NULL)
    return;

  int len = strlen(s);
  if (len <= 0)
    return;

  if (available(self, len) <= 0)
    return;

  strcat(self->buf, s);
  self->len += len;
}

void strbuf_vnappend(StrBuf *self, int count, va_list args)
{
  char *s;
  while (count-- > 0) {
    s = va_arg(args, char *);
    strbuf_append(self, s);
  }
}

void strbuf_vappend(StrBuf *self, int count, ...)
{
  va_list args;
  va_start(args, count);
  strbuf_vnappend(self, count, args);
  va_end(args);
}

void strbuf_append_char(StrBuf *self, char ch)
{
  if (available(self, 1) <= 0)
    return;
  self->buf[self->len++] = ch;
}

void strbuf_append_int(StrBuf *self, int64_t val)
{
  char buf[64];
  snprintf(buf, 63, "%ld", val);
  strbuf_append(self, buf);
}

void strbuf_append_float(StrBuf *self, double val)
{
  char buf[64];
  snprintf(buf, 63, "%.8f", val);
  strbuf_append(self, buf);
}
