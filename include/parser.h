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

/* token's position */
struct pos {
  /* row */
  int row;
  /* column */
  int col;
};

struct module {
};

struct parserstate {
  /* file name */
  char *filename;
  /* its module */
  struct module *mod;

  /* is interactive ? */
  int interactive;
  /* quit interactive mode ? */
  int quit;
  /* is complete statement ? */
  int more;
  /* token */
  int token;
  /* token length */
  int len;
  /* token position */
  struct pos pos;

};

#include "koala_yacc.h"

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
