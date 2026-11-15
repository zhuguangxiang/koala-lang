/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include <pthread.h>
#include <strings.h>
#include <time.h>
#include "common.h"

#ifndef NOLOG

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Logger {
    const char *name;
    LogLevel level;
    int quiet;
    FILE *filp;
    pthread_mutex_t lock;
    pthread_mutex_t flock;
} Logger;

static Logger logger = {
    .name = NULL,
    .level = LOG_DEBUG,
    .quiet = 0,
    .filp = NULL,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .flock = PTHREAD_MUTEX_INITIALIZER,
};

static char *level_names[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

static const char *log_getenv(const char *name, const char *suffix)
{
    char key[64];
    int n = 0;
    for (const char *s = name; *s && n < 48; s++) {
        key[n++] = isalnum((unsigned char)*s) ? toupper((unsigned char)*s) : '_';
    }
    snprintf(key + n, sizeof(key) - n, "%s", suffix);
    return getenv(key);
}

static LogLevel log_parse_level(const char *s, LogLevel dflt)
{
    if (!s || !*s) return dflt;
    for (int i = LOG_TRACE; i <= LOG_FATAL; i++) {
        if (!strcasecmp(s, level_names[i])) return (LogLevel)i;
    }
    fprintf(stderr, "invalid log level '%s', use %s\n", s, level_names[dflt]);
    return dflt;
}

void init_log(const char *name, LogLevel default_level, const char *file, int quiet)
{
    logger.name = name;
    logger.level = log_parse_level(log_getenv(name, "_LOG_LEVEL"), default_level);
    logger.quiet = quiet;

    const char *env_file = log_getenv(name, "_LOG");
    if (env_file && *env_file) file = env_file;
    if (file) {
        logger.filp = fopen(file, "a");
        if (!logger.filp) fprintf(stderr, "cannot open log file '%s'\n", file);
    }
}

void fini_log(void)
{
    if (logger.filp) fclose(logger.filp);
    logger.filp = NULL;
}

#ifdef LOG_COLOR
static char *level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m",
                                "\x1b[33m", "\x1b[31m", "\x1b[35m" };
#endif

void _log_log(LogLevel level, char *file, int line, char *fmt, ...)
{
    if (level < logger.level) return;

    char tag[40] = { 0 };
    if (logger.name) snprintf(tag, sizeof(tag), "[%s] ", logger.name);

    /* get current time */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    /* log to stderr */
    if (!logger.quiet) {
        va_list args;
        char buf[16];
        buf[strftime(buf, sizeof(buf) - 1, "%H:%M:%S", tm)] = '\0';

        char buf2[128] = { 0 };

#ifdef LOG_COLOR
        snprintf(buf2, 127, "\x1b[90m%s:%d:\x1b[0m", file, line);

        pthread_mutex_lock(&logger.lock);
        fprintf(stderr, "%s %s%s%-5s\x1b[0m %-20s ", buf, tag, level_colors[level],
                level_names[level], buf2);
#else
        snprintf(buf2, 127, "%s:%d:", file, line);

        pthread_mutex_lock(&logger.lock);
        fprintf(stderr, "%s %s%-5s %-20s ", buf, tag, level_names[level], buf2);
#endif

        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);

        pthread_mutex_unlock(&logger.lock);
    }

    /* log to file */
    if (logger.filp) {
        va_list args;
        char buf[32];
        buf[strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S", tm)] = '\0';

        char buf2[128] = { 0 };
        snprintf(buf2, 127, "%s:%d:", file, line);

        pthread_mutex_lock(&logger.flock);
        fprintf(logger.filp, "%s %s%-5s %-20s ", buf, tag, level_names[level], buf2);

        va_start(args, fmt);
        vfprintf(logger.filp, fmt, args);
        va_end(args);
        fprintf(logger.filp, "\n");
        fflush(logger.filp);

        pthread_mutex_unlock(&logger.flock);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* !NOLOG */
