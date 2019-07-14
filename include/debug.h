/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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

#else /* !NDEBUG */

#define __NAME__                        \
({                                      \
  char *slash = strrchr(__FILE__, '/'); \
  slash != NULL ? slash + 1 : __FILE__; \
})

#define __DBG_COLOR__   "\x1b[36mdebug:\x1b[0m "
#define debug(fmt, ...) __print__(fmt, __DBG_COLOR__, ##__VA_ARGS__)
#define __WARN_COLOR__  "\x1b[33mwarn:\x1b[0m "
#define warn(fmt, ...)  __print__(fmt, __WARN_COLOR__, ##__VA_ARGS__)

#endif /* NDEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DEBUG_H_ */
