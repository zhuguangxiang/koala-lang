
#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3

extern int loglevel;

#define __debug(format, ...) printf(format, ##__VA_ARGS__)

#ifdef LOG_NDEBUG
#define debug(format, ...) ((void)0)
#else
#define debug(format, ...) do { \
	__debug("[DEBUG][%s:%d]" format "\n", \
	__FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)
#endif

#define info(format, ...) do { \
	if (loglevel >= LOG_LEVEL_INFO) \
		__debug("[INFO]" format "\n", ##__VA_ARGS__); \
} while (0)

#define warn(format, ...) do { \
	if (loglevel >= LOG_LEVEL_WARN) \
		__debug("[WARN]" format "\n", ##__VA_ARGS__); \
} while (0)

#define error(format, ...) do { \
	if (loglevel >= LOG_LEVEL_ERROR) \
		__debug("[ERROR]" format "\n", ##__VA_ARGS__);\
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_LOG_H_ */
