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

#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __FILENAME__ \
({ \
  char *slash = strrchr(__FILE__, '/'); \
  slash != NULL ? slash + 1 : __FILE__; \
})

typedef enum loglevel {
  LOG_TRACE = 0, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
} LogLevel;

void Log_Log(LogLevel level, char *file, int line, char *fmt, ...);

#define Log_Trace(...) Log_Log(LOG_TRACE, __FILENAME__, __LINE__, __VA_ARGS__)
#define Log_Debug(...) Log_Log(LOG_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__)
#define Log_Info(...) Log_Log(LOG_INFO, __FILENAME__, __LINE__, __VA_ARGS__)
#define Log_Warn(...) Log_Log(LOG_WARN, __FILENAME__, __LINE__, __VA_ARGS__)
#define Log_Error(...) Log_Log(LOG_ERROR, __FILENAME__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) Log_Log(LOG_FATAL, __FILENAME__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_LOG_H_ */
