/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_COMMON_H_
#define _KOALA_COMMON_H_

#include <assert.h>
#include <errno.h>
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
#define ALIGN(x, n) (((x) + (n) - 1) & ~((n) - 1))

/* Get the aligned pointer size */
#define ALIGN_PTR(x) ALIGN(x, sizeof(uintptr_t))

/* Get the number of elements in an array */
#define COUNT_OF(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

/* pointer to integer */
#define PTR2INT(p) ((int)(uintptr_t)(p))

/* integer to pointer */
#define INT2PTR(i) ((void *)(uintptr_t)(i))

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

#define UNREACHABLE() do { \
    fprintf(stderr, "%s:%d: Why goes here?\n", __FILE_NAME__, __LINE__); \
    abort(); \
} while (0)

#define NIY() do { \
    fprintf(stderr, "%s:%d: Not Yet Implemented\n", __FILE_NAME__, __LINE__); \
    abort(); \
} while (0)

/* clang-format on */

#ifndef NDEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) ((void)(0))
#endif

/* largest positive value of type ssize_t. */
#define SSIZE_MAX ((ssize_t)(((size_t) - 1) >> 1))
/* Smallest negative value of type ssize_t. */
#define SSIZE_MIN (-SSIZE_MAX - 1)

/* clang-format off */
#define FFS(x) ({ int v = __builtin_ffs(x); ASSERT(v > 0); v; })
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COMMON_H_ */
