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

/* Simple dynamic string buffer. */

#ifndef _KOALA_STRBUF_H_
#define _KOALA_STRBUF_H_

#include <stdarg.h>
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* string buffer */
typedef struct strbuf {
  /* allocated size */
  int size;
  /* used size */
  int len;
  /* contains string */
  char *buf;
} StrBuf;

/* Declare a string buffer. */
#define STRBUF(name) \
  StrBuf name = {0, 0, NULL}

/* Free a string buffer. */
static inline void strbuf_fini(StrBuf *self)
{
  kfree(self->buf);
  memset(self, 0, sizeof(*self));
}

/* Write string with len */
void strbuf_nappend(StrBuf *self, char *s, int len);
/* Write a null-terminated string. */
void strbuf_append(StrBuf *self, char *s);

/* Write 'count' null-terminated strings. */
void strbuf_vappend(StrBuf *self, int count, ...);
void strbuf_vnappend(StrBuf *self, int count, va_list args);

/* Write a character. */
void strbuf_append_char(StrBuf *self, char ch);

/* Write an integer. */
void strbuf_append_int(StrBuf *self, long long val);

/* Write a float. */
void strbuf_append_float(StrBuf *self, double val);

/* Get null-terminated string from the string buffer. */
static inline char *strbuf_tostr(StrBuf *self)
{
  return self->buf;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRBUF_H_ */
