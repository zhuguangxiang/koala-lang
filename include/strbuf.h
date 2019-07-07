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

#define STRBUF(name) \
  struct strbuf name = {0, 0, NULL}

static inline void strbuf_free(struct strbuf *self)
{
  kfree(self->buf);
  memset(self, 0, sizeof(*self));
}

void strbuf_write(struct strbuf *self, char *s);
void strbuf_write_format(struct strbuf *self, char *fmt, ...);
void strbuf_write_char(struct strbuf *self, char ch);
char *atom_string(char *s);
char *atom_find(char *s);
void atom_init(void);
void atom_free(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STRBUF_H_ */
