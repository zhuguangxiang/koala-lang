/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common.h"
#include "log.h"
#include "config.h"

static char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_COLOR
static char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif

void Log_Log(Logger *log, LogLevel level, char *file, int line, char *fmt, ...)
{
  if (level < log->level)
    return;

  /* log to stderr */
  if (!log->quiet) {
    va_list args;
    char buf[16];
    /* get current time */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", tm)] = '\0';
#ifdef LOG_COLOR
    fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
            buf, level_colors[level], level_names[level], file, line);
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
  if (log->filp) {
    va_list args;
    char buf[32];
    /* get current time */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm)] = '\0';
    fprintf(log->filp, "%s %-5s %s:%d: ",
            buf, level_names[level], file, line);
    va_start(args, fmt);
    vfprintf(log->filp, fmt, args);
    va_end(args);
    fprintf(log->filp, "\n");
    fflush(log->filp);
  }
}
