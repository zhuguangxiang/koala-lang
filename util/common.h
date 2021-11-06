/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

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

/* types */

/* clang-format off */

typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef int64_t     int64;
typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;
typedef uint64_t    uint64;
typedef uintptr_t   uintptr;
typedef float       float32;
typedef double      float64;
typedef int8_t      bool;
typedef uintptr_t   anyref;
typedef uintptr_t   objref;

#define true    1
#define false   0
#define null    NULL

/* clang-format on */

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

/* clang-format on */

#define PTR_SIZE sizeof(void *)

#define OBJ_SIZE(ptr) sizeof(*ptr)

typedef void (*initfunc)(void);

/* clang-format off */
#define INIT_FUNC_LEVEL_0(name) \
    static initfunc __##name    \
    __attribute__((used, section("_init_func_0"))) = name

#define INIT_FUNC_LEVEL_1(name) \
    static initfunc __##name    \
    __attribute__((used, section("_init_func_1"))) = name

#define INIT_FUNC_LEVEL_2(name) \
    static initfunc __##name    \
    __attribute__((used, section("_init_func_2"))) = name
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COMMON_H_ */
