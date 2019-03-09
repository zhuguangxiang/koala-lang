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

#ifndef _KOALA_STRINGBUF_H_
#define _KOALA_STRINGBUF_H_

#include "atomstring.h"
#include "mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stringbuf {
  int length;
  int size;
  char *data;
} StringBuf;

void __StringBuf_Append_CStr(StringBuf *buf, char *s);
void __StringBuf_Append(StringBuf *buf, String s);
void __StringBuf_Format(StringBuf *buf, int cstr, char *fmt, ...);
void __StringBuf_Append_Char(StringBuf *buf, char ch);

#define DeclareStringBuf(name) StringBuf name = {0, 0, NULL};

#define FiniStringBuf(name) \
({                          \
  Mfree((name).data);       \
  (name).length = 0;        \
  (name).size = 0;          \
  (name).data = NULL;       \
})

/* fmt: "#.#" */
#define StringBuf_Format(name, fmt, ...) \
  __StringBuf_Format(&(name), 0, fmt, __VA_ARGS__)

#define StringBuf_Format_CStr(name, fmt, ...) \
  __StringBuf_Format(&(name), 1, fmt, __VA_ARGS__)

#define StringBuf_Append(name, s) \
  __StringBuf_Append(&(name), s)

#define StringBuf_Append_CStr(name, s) \
  __StringBuf_Append_CStr(&(name), s)

#define StringBuf_Append_Char(name, ch) \
  __StringBuf_Append_Char(&(name), ch)

#define AtomString_Format(fmt, ...)             \
({                                              \
  char *s;                                      \
  DeclareStringBuf(buf);                        \
  StringBuf_Format_CStr(buf, fmt, __VA_ARGS__); \
  s = AtomString_New(buf.data).str;             \
  FiniStringBuf(buf);                           \
  s;                                            \
})

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STRINGBUF_H_ */
