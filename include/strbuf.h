/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Simple dynamic string buffer.
 */

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

/* Write a null-terminated string. */
void strbuf_append(StrBuf *self, char *s);

/* Write 'count' null-terminated strings. */
void strbuf_vappend(StrBuf *self, int count, ...);
void strbuf_vnappend(StrBuf *self, int count, va_list args);

/* Write a character. */
void strbuf_append_char(StrBuf *self, char ch);

/* Get null-terminated string from the string buffer. */
static inline char *strbuf_tostr(StrBuf *self)
{
  return self->buf;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_STRBUF_H_ */
