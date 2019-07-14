/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "strbuf.h"

#define EXPAND_MIN_SIZE 32

static int expand(struct strbuf *self, int min)
{
  int size = self->len + min + 1;
  int newsize = self->size;
  while (newsize <= size)
    newsize += EXPAND_MIN_SIZE;

  char *newbuf = kmalloc(newsize);
  if (!newbuf)
    return -1;

  if (self->buf) {
    strcpy(newbuf, self->buf);
    kfree(self->buf);
  }
  self->buf  = newbuf;
  self->size = newsize;
  return 0;
}

static int available(struct strbuf *self, int size)
{
  int left = self->size - self->len - 1;
  if (left <= size && expand(self, size))
    return -1;
  return self->size - self->len - 1 - size;
}

void strbuf_append(struct strbuf *self, char *s)
{
  int len = strlen(s);
  if (len <= 0)
    return;

  if (available(self, len) <= 0)
    return;

  strcat(self->buf, s);
  self->len += len;
}

void strbuf_vnappend(struct strbuf *self, int n, va_list args)
{
  char *s;
  while (n-- > 0) {
    s = va_arg(args, char *);
    strbuf_append(self, s);
  }
}

void strbuf_vappend(struct strbuf *self, int n, ...)
{
  va_list args;
  va_start(args, n);
  strbuf_vnappend(self, n, args);
  va_end(args);
}

void strbuf_append_char(struct strbuf *self, char ch)
{
  if (available(self, 1) <= 0)
    return;

  self->buf[self->len++] = ch;
}
