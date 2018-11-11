
#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#include <stdio.h>
#include "printcolor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3

extern int loglevel;

#define __debug(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#define COLOR_MSG(fmt) COLOR_WHITE "%s:%d\t%6s: " fmt COLOR_NC "\n"
#define ERROR_MSG(fmt) \
  COLOR_LIGHT_WHITE "%s:%d\t" COLOR_LIGHT_RED "%6s: " COLOR_NC fmt "\n"

#ifdef LOG_DEBUG
#define debug(format, ...) do { \
  __debug(COLOR_MSG(format), __FILE__, __LINE__, "debug", ##__VA_ARGS__); \
} while (0)
#else
#define debug(format, ...) ((void)0)
#endif

#define info(format, ...) do { \
  if (loglevel >= LOG_LEVEL_INFO) \
    __debug(COLOR_MSG(format), __FILE__, __LINE__, "info", ##__VA_ARGS__); \
} while (0)

#define warn(format, ...) do { \
  if (loglevel >= LOG_LEVEL_WARN) \
    __debug(COLOR_MSG(format), __FILE__, __LINE__, "warn", ##__VA_ARGS__); \
} while (0)

#define error(format, ...) do { \
  if (loglevel >= LOG_LEVEL_ERROR) \
    __debug(ERROR_MSG(format), __FILE__, __LINE__, "error", ##__VA_ARGS__); \
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_LOG_H_ */
