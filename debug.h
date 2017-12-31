
#ifndef _KOALA_DEBUG_H_
#define _KOALA_DEBUG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_LEVEL_INFO  1
#define DEBUG_LEVEL_WARN  2
#define DEBUG_LEVEL_ERROR 3

extern int debug_level;

#define debug(format, ...) printf(format, ##__VA_ARGS__)

#define debug_info(format, ...) do {        \
  if (debug_level >= DEBUG_LEVEL_INFO)      \
    debug("[INFO]"#format, ## __VA_ARGS__); \
} while (0)

#define debug_warn(format, ...) do {        \
  if (debug_level >= DEBUG_LEVEL_WARN)      \
    debug("[WARN]"#format, ## __VA_ARGS__); \
} while (0)

#define debug_error(format, ...) do {       \
  if (debug_level >= DEBUG_LEVEL_ERROR)     \
    debug("[ERROR]"#format, ## __VA_ARGS__);\
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_DEBUG_H_ */
