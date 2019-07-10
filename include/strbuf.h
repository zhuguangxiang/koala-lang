/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _KOALA_STRBUF_H_
#define _KOALA_STRBUF_H_

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* string buffer */
struct strbuf {
  /* allocated size */
  int size;
  /* used size */
  int len;
  /* contains string */
  char *buf;
};

/*
 * Declare a string buffer.
 */
#define STRBUF(name) \
  struct strbuf name = {0, 0, NULL}

/*
 * Free a string buffer.
 *
 * self - The string buffer to free
 *
 * Returns nothing.
 */
static inline void strbuf_free(struct strbuf *self)
{
  kfree(self->buf);
  memset(self, 0, sizeof(*self));
}

/*
 * Write a 0-terminated string.
 *
 * self - The string buffer to write.
 * s    - The string to be written.
 *
 * Returns nothing.
 */
void strbuf_append(struct strbuf *self, char *s);

/*
 * Write a character.
 *
 * self - The string buffer to write.
 * s    - The character to be written.
 *
 * Returns nothing.
 */
void strbuf_append_char(struct strbuf *self, char ch);

/*
 * Get 0-terminated string from the string buffer.
 *
 * self - The string buffer to be get a string.
 *
 * Returns a 0-terminated string.
 */
static inline char *strbuf_tostr(struct strbuf *self)
{
  return self->buf;
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STRBUF_H_ */
