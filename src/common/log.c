/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include <time.h>
#include "common.h"

typedef struct _Logger {
    LogLevel level;
    int quiet;
    FILE *filp;
} Logger;

static Logger logger = {
    .level = LOG_DEBUG,
    .quiet = 0,
    .filp = NULL,
};

void init_log(LogLevel default_level, const char *file, int quiet)
{
    logger.level = default_level;
    logger.quiet = quiet;
    logger.filp = fopen(file, "wa");
}

static char *level_names[] = { "TRACE", "DEBUG", "INFO",
                               "WARN",  "ERROR", "FATAL" };

#ifdef LOG_COLOR
static char *level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m",
                                "\x1b[33m", "\x1b[31m", "\x1b[35m" };
#endif

void _log_log(LogLevel level, char *file, int line, char *fmt, ...)
{
    if (level < logger.level) return;

    /* get current time */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    /* log to stderr */
    if (!logger.quiet) {
        va_list args;
        char buf[16];
        buf[strftime(buf, sizeof(buf) - 1, "%H:%M:%S", tm)] = '\0';
#ifdef LOG_COLOR
        fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
                level_colors[level], level_names[level], file, line);
#else
        fprintf(stderr, "%s %-5s %s:%d: ", buf, level_names[level], file, line);
#endif
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    /* log to file */
    if (logger.filp) {
        va_list args;
        char buf[32];
        buf[strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S", tm)] = '\0';
        fprintf(logger.filp, "%s %-5s %s:%d: ", buf, level_names[level], file,
                line);
        va_start(args, fmt);
        vfprintf(logger.filp, fmt, args);
        va_end(args);
        fprintf(logger.filp, "\n");
        fflush(logger.filp);
    }
}
