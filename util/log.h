/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_LOG_H_
#define _KOALA_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "color.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */

#define _print_(clr, fmt, ...) fprintf(stdout, clr fmt "\n", ##__VA_ARGS__)

#ifndef NLOG

#define debug(fmt, ...) _print_(DBG_COLOR, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  _print_(WARN_COLOR, fmt, ##__VA_ARGS__)
#define error(fmt, ...) _print_(ERR_COLOR, fmt, ##__VA_ARGS__)
#define log(fmt, ...)   fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#else /* NLOG */

#define debug(fmt, ...) ((void)0)
#define warn(fmt, ...)  ((void)0)
#define error(fmt, ...) ((void)0)
#define log(fmt, ...)   ((void)0)

#endif /* !NLOG */

#define panic(fmt, ...) _print_(PANIC_COLOR, fmt, ##__VA_ARGS__); abort();

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LOG_H_ */
