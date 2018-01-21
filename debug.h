
#ifndef _KOALA_DEBUG_H_
#define _KOALA_DEBUG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_WARN  2
#define DEBUG_LEVEL_INFO  3

extern int debug_level;

#define __debug(format, ...) printf(format, ##__VA_ARGS__)

#ifdef NDEBUG
#define debug(format, ...) ((void)0)
#else
#define debug(format, ...) do {             \
  __debug("[DEBUG]"format, ##__VA_ARGS__)   \
} while (0)
#endif

#define debug_info(format, ...) do {        \
  if (debug_level >= DEBUG_LEVEL_INFO)      \
    __debug("[INFO]"format, ##__VA_ARGS__); \
} while (0)

#define debug_warn(format, ...) do {        \
  if (debug_level >= DEBUG_LEVEL_WARN)      \
    __debug("[WARN]"format, ##__VA_ARGS__); \
} while (0)

#define debug_error(format, ...) do {       \
  if (debug_level >= DEBUG_LEVEL_ERROR)     \
    __debug("[ERROR]"format, ##__VA_ARGS__);\
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_DEBUG_H_ */
