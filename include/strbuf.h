/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_STRBUF_H_
#define _KOALA_STRBUF_H_

#include <stdarg.h>
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
 * Write 'n' 0-terminated strings.
 *
 * self - The string buffer to write.
 * n    - The number of strings to be written.
 *
 * Returns nothing.
 */
void strbuf_vnappend(struct strbuf *self, int n, va_list args);
void strbuf_nappend(struct strbuf *self, int n, ...);

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
