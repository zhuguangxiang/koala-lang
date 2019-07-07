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

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* position */
struct pos { int row; int col; };

struct module {
  /* symbol table per module, not per source file */
  struct symboltable *stbl;
};

/* per source file */
struct parserstate {
  /* file name */
  char *filename;
  /* its module */
  struct module *mod;

  /* is interactive ? */
  int interactive;
  /* is complete ? */
  int more;
  /* token */
  int token;
  /* token length */
  int len;
  /* token position */
  struct pos pos;

  /* error numbers */
  int errnum;
};

#define MAX_ERRORS 8

#define syntax_error(ps, pos, fmt, ...)                        \
({                                                             \
  if (++ps->errnum > MAX_ERRORS) {                             \
    /* more than MAX_ERRORS, discard left errors shown */      \
    fprintf(stderr, "%s: " __ERR_COLOR__ "Too many errors.\n", \
            ps->filename);                                     \
  } else {                                                     \
    fprintf(stderr, "%s:%d:%d: " __ERR_COLOR__ fmt "\n",       \
            ps->filename, pos->row, pos->col, __VA_ARGS__);    \
  }                                                            \
})

#include "koala_yacc.h"

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
