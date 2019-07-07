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

#ifndef _KOALA_DEBUG_H_
#define _KOALA_DEBUG_H_

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __print__(fmt, clr, ...) \
  fprintf(stderr, "%s:%d: " clr fmt "\n", __NAME__, __LINE__, ##__VA_ARGS__)

#define __ERR_COLOR__ "\x1b[31merror:\x1b[0m "
#define error(fmt, ...) __print__(fmt, __ERR_COLOR__, ##__VA_ARGS__)
#define errmsg(fmt, ...) \
  fprintf(stderr, __ERR_COLOR__ fmt "\n", ##__VA_ARGS__)

#ifdef NDEBUG

/* disabled */
#define debug(fmt, ...) ((void)0)
#define warn(fmt, ...)  ((void)0)

#else

#define __NAME__                        \
({                                      \
  char *slash = strrchr(__FILE__, '/'); \
  slash != NULL ? slash + 1 : __FILE__; \
})

#define __DBG_COLOR__   "\x1b[36mdebug:\x1b[0m "
#define debug(fmt, ...) __print__(fmt, __DBG_COLOR__, ##__VA_ARGS__)
#define __WARN_COLOR__  "\x1b[33mwarn:\x1b[0m "
#define warn(fmt, ...)  __print__(fmt, __WARN_COLOR__, ##__VA_ARGS__)

#endif

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_DEBUG_H_ */
