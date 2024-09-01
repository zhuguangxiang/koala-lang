/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _LogLevel {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

void init_log(LogLevel default_level, const char *file, int quiet);
void fini_log(void);
void _log_log(LogLevel level, char *file, int line, char *fmt, ...);

#define log_error(...) _log_log(LOG_ERROR, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_fatal(...) _log_log(LOG_FATAL, __FILE_NAME__, __LINE__, __VA_ARGS__)

#ifndef NOLOG

#define log_trace(...) _log_log(LOG_TRACE, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_debug(...) _log_log(LOG_DEBUG, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_info(...)  _log_log(LOG_INFO, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_warn(...)  _log_log(LOG_WARN, __FILE_NAME__, __LINE__, __VA_ARGS__)

#else

#define log_trace(...) ((void)(0))
#define log_debug(...) ((void)(0))
#define log_info(...)  ((void)(0))
#define log_warn(...)  ((void)(0))

#endif

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOG_H_ */
