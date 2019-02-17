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

#ifndef _KOALA_OPTIONS_H_
#define _KOALA_OPTIONS_H_

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct namevalue {
  char *name;
  char *value;
};

typedef struct options {
  /* -h or ? */
  void (*usage)(char *name);
  /* -v version */
  void (*version)(void);
  /* -s */
  char *srcpath;
  /* -p */
  Vector pathes;
  /* search path string */
  char *pathstrings;
  /* -o output dir */
  char *outpath;
  /* -D name=value */
  Vector nvs;
  /* file(klc) or dir(package) names */
  Vector names;
} Options;

int init_options(Options *opts, void (*usage)(char *), void (*version)(void));
void fini_options(Options *opts);
void parse_options(int argc, char *argv[], Options *opts);
int options_number(Options *opts);
void options_toarray(Options *opts, char *array[], int ind);
void show_options(Options *opts);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPTIONS_H_ */
