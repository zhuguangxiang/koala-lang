/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "stringbuf.h"

#define EXPAND_SIZE 32

static int expand(StringBuf *buf, int min)
{
  int size = buf->length + min + 1;
  int bufsize = buf->size;
  while (size > bufsize)
    bufsize += EXPAND_SIZE;

  char *data = mm_alloc(bufsize);
  if (data == NULL)
    return -1;

  if (buf->data != NULL) {
    strcpy(data, buf->data);
    mm_free(buf->data);
  }
  buf->data = data;
  buf->size = bufsize;
  return 0;
}

static int available(StringBuf *buf, int min)
{
  int left = buf->size - buf->length - 1;
  if (left < min) {
    int result = expand(buf, min);
    if (result != 0)
      return -1;
  }

  return buf->size - buf->length - 1 - min;
}

void __StringBuf_Append(StringBuf *buf, String s)
{
  int len = AtomString_Length(s);
  if (len <= 0)
    return;

  if (available(buf, len) <= 0)
    return;

  strcat(buf->data, s.str);
  buf->length += len;
}

void __StringBuf_Append_Char(StringBuf *buf, char ch)
{
  if (available(buf, 1) <= 0)
    return;

  buf->data[buf->length] = ch;
  buf->length += 1;
}

void __StringBuf_Format(StringBuf *buf, int cstr, char *fmt, ...)
{
  String s;
  va_list args;
  va_start(args, fmt);
  char ch;
  while ((ch = *fmt++)) {
    if (ch == '#') {
      if (cstr) {
        s.str = va_arg(args, char *);
      } else {
        s = va_arg(args, String);
      }
      __StringBuf_Append(buf, s);
    } else {
      __StringBuf_Append_Char(buf, ch);
    }
  }
  va_end(args);
}
