/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _FILENAME_                      \
({                                      \
  char *slash = strrchr(__FILE__, '/'); \
  slash != NULL ? slash + 1 : __FILE__; \
})

#define _print_(clr, fmt, ...) \
  fprintf(stdout, clr fmt "\n", ##__VA_ARGS__)

#define _ERR_COLOR_   "\x1b[1;31merror:\x1b[0m "
#define _PANIC_COLOR_ "\x1b[1;31mpanic:\x1b[0m "
#define error(fmt, ...) _print_(_ERR_COLOR_, fmt, ##__VA_ARGS__)

#define panic(fmt, ...)                   \
do {                                      \
  _print_(_PANIC_COLOR_, "%s:%d: " fmt,   \
    _FILENAME_, __LINE__, ##__VA_ARGS__); \
  abort();                                \
} while (0)

#define expect(expr)                                      \
do {                                                      \
  if (!(expr)) {                                          \
    _print_(_ERR_COLOR_, "%s:%d: expect '%s' to be true", \
      _FILENAME_, __LINE__, #expr);                       \
  }                                                       \
} while (0)

#ifdef NLog

/* disabled */
#define debug(fmt, ...) ((void)0)
#define warn(fmt, ...)  ((void)0)
#define print(ftm, ...) ((void)0)

#else /* !NLog */

#define _DBG_COLOR_   "\x1b[1;36mdebug:\x1b[0m "
#define _WARN_COLOR_  "\x1b[1;35mwarning:\x1b[0m "
#define debug(fmt, ...) _print_(_DBG_COLOR_, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  _print_(_WARN_COLOR_, fmt, ##__VA_ARGS__)
#define print(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

#endif /* NLog */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOG_H_ */
