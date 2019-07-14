/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <assert.h>
#include "strbuf.h"

int main(int argc, char *argv[])
{
  STRBUF(sbuf);

  char *s = "Hello, Koala";
  strbuf_append(&sbuf, s);
  strbuf_append(&sbuf, s);
  strbuf_append(&sbuf, s);
  assert(sbuf.len == 36 && sbuf.size == 64);

  strbuf_append_char(&sbuf, 'O');
  strbuf_append_char(&sbuf, 'K');
  assert(sbuf.len == 38 && sbuf.size == 64);

  s = strbuf_tostr(&sbuf);
  assert(!strcmp(s, "Hello, KoalaHello, KoalaHello, KoalaOK"));

  strbuf_free(&sbuf);
  return 0;
}
