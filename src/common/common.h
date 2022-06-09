/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_COMMON_H_
#define _KOALA_COMMON_H_

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get the min(max) one of the two numbers */
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Get the aligned value */
#define ALIGN(x, n) (((x) + (n)-1) & ~((n)-1))

/* Get the aligned pointer size */
#define ALIGN_PTR(x) ALIGN(x, sizeof(uintptr))

/* Get the number of elements in an array */
#define COUNT_OF(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

/* pointer to integer */
#define PTR2INT(p) ((int)(uintptr)(p))

/* integer to pointer */
#define INT2PTR(i) ((void *)(uintptr)(i))

/* clang-format off */

/* endian check */
#define CHECK_BIG_ENDIAN ({ int _i = 1; !*((char *)&_i); })

/*
 * Get the 'type' pointer from the pointer to `member`
 * which is embedded inside the 'type'
 */
#define CONTAINER_OF(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})

#define PTR_SIZE sizeof(void *)
#define OBJ_SIZE(ptr) sizeof(*ptr)

#define UNUSED(expr) do { (void)(expr); } while (0)

/* clang-format on */

#define BOLD_COLOR(x) "\033[1m" x "\x1b[0m"
#define RED_COLOR(x)  "\x1b[31m" x "\x1b[0m"

#define DEBUG_PREFIX "\x1b[36mdebug: \x1b[0m "
#define WARN_PREFIX  "\x1b[35mwarn: \x1b[0m "
#define ERROR_PREFIX "\x1b[31merror: \x1b[0m "

#define println(fmt, ...)   fprintf(stdout, fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) println(ERROR_PREFIX fmt, ##__VA_ARGS__)

#ifndef NLOG
#define log_debug(fmt, ...) println(DEBUG_PREFIX fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  println(WARN_PREFIX fmt, ##__VA_ARGS__)
#else /* NLOG */
#define log_debug(fmt, ...) ((void)0)
#define log_warn(fmt, ...)  ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COMMON_H_ */
