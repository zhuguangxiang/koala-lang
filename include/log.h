/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NLOG
/* Log is disabled. */
#define log_trace(...) ((void)0)
#define log_debug(...) ((void)0)
#define log_warn(...)  ((void)0)
#define log_error(...) ((void)0)
#define log_fatal(...) ((void)0)
#define outf(fmt, ...) ((void)0)
#define outs(msg)      ((void)0)
#define log_config(quiet, level, file) ((void)0)
#else
/* Log is enabled. */
enum { LOG_TRACE, LOG_DEBUG, LOG_WARN, LOG_ERROR, LOG_FATAL };
#define __FILE_NAME__ \
({                    \
  char *slash = strrchr(__FILE__, '/'); \
  slash != NULL ? slash + 1 : __FILE__; \
})
#define log_trace(...) log_log(LOG_TRACE, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define outf(fmt, ...) log_output(fmt, __VA_ARGS__)
#define outs(msg)      log_output(msg)
/*
 * Configure the logger.
 *
 * quiet - No any output to standard terminal.
 * level - Current log level, see enum LOG_XX.
 * file  - The file to be printed. If it's null, not print into file.
 *
 * Returns nothing.
 */
void log_config(int quiet, int level, char *file);
/* internal use, please use log_xxx */
void log_log(int level, char *file, int line, char *fmt, ...);
/* internal use, please outf & outs */
void log_output(char *fmt, ...);
#endif

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_LOG_H_ */
