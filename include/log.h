/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __print__(clr, fmt, ...) \
  fprintf(stdout, clr fmt "\n", ##__VA_ARGS__)

#define __ERR_COLOR__   "\x1b[1;31merror:\x1b[0m "
#define __PANIC_COLOR__ "\x1b[1;31mfatal:\x1b[0m "
#define error(fmt, ...) __print__(__ERR_COLOR__, fmt, ##__VA_ARGS__)
#define panic(fmt, ...) ({                        \
  __print__(__PANIC_COLOR__, fmt, ##__VA_ARGS__); \
  abort();                                        \
})

#ifdef NDEBUG

/* disabled */
#define debug(fmt, ...) ((void)0)
#define warn(fmt, ...)  ((void)0)
#define print(ftm, ...) ((void)0)

#else /* !NDEBUG */

#define __FILE_NAME__                   \
({                                      \
  char *slash = strrchr(__FILE__, '/'); \
  slash != NULL ? slash + 1 : __FILE__; \
})

#define __DBG_COLOR__   "\x1b[1;36mdebug:\x1b[0m "
#define debug(fmt, ...) __print__(__DBG_COLOR__, fmt, ##__VA_ARGS__)
#define __WARN_COLOR__  "\x1b[1;35mwarning:\x1b[0m "
#define warn(fmt, ...)  __print__(__WARN_COLOR__, fmt, ##__VA_ARGS__)
#define print(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)

#endif /* NDEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOG_H_ */
