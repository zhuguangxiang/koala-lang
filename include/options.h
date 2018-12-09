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

struct options {
  /* -h or ? */
  void (*usage)(char *name);
  /* -v version */
  void (*version)(void);
  /* -s */
  char *srcpath;
  /* -p */
  Vector pathes;
  /* -o output dir */
  char *outpath;
  /* -D */
  Vector nvs;
  /* file(klc) or dir(package) names */
  Vector names;
};

int init_options(struct options *opts);
void fini_options(struct options *opts);
#define set_options_usage(opts, usagefunc) \
do { \
  (opts)->usage = usagefunc; \
} while (0)
#define set_options_version(opts, versionfunc) \
do { \
  (opts)->version = versionfunc; \
} while (0)
void parse_options(int argc, char *argv[], struct options *opts);
int options_number(struct options *opts);
void options_toarray(struct options *opts, char *array[], int ind);
void show_options(struct options *opts);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPTIONS_H_ */
