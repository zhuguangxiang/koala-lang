/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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

#ifndef NOLOG

/*
 * Only built in debug_log.
 * name: program name, printed as "[name]" in each log line.
 * Env vars (name upper-cased, e.g. KOALAC_LOG) override the arguments:
 *   <NAME>_LOG        log file path
 *   <NAME>_LOG_LEVEL  trace|debug|info|warn|error|fatal
 */
void init_log(const char *name, LogLevel default_level, const char *file, int quiet);
void fini_log(void);
void _log_log(LogLevel level, char *file, int line, char *fmt, ...);

#define log_trace(...) _log_log(LOG_TRACE, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_debug(...) _log_log(LOG_DEBUG, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_info(...)  _log_log(LOG_INFO, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_warn(...)  _log_log(LOG_WARN, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_error(...) _log_log(LOG_ERROR, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_fatal(...) _log_log(LOG_FATAL, __FILE_NAME__, __LINE__, __VA_ARGS__)

#else

#include <stdio.h>  // IWYU pragma: export
#include <stdlib.h> // IWYU pragma: export

#define init_log(name, default_level, file, quiet) ((void)0)
#define fini_log()                                 ((void)0)

#define log_trace(...) ((void)(0))
#define log_debug(...) ((void)(0))
#define log_info(...)  ((void)(0))
#define log_warn(...)  ((void)(0))

#define _log_abort(level, fmt, ...) \
    (fprintf(stderr, "%s %s:%d: " fmt "\n", level, __FILE_NAME__, __LINE__, ##__VA_ARGS__), abort())
#define log_error(fmt, ...) _log_abort("ERROR", fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) _log_abort("FATAL", fmt, ##__VA_ARGS__)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOG_H_ */
