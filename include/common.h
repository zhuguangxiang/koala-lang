/*===-- common.h - Common Utils -----------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares common data structure, utils and macros.              *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_COMMON_H_
#define _KOALA_COMMON_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLLEXPORT __attribute__((visibility("default")))

/* endian check */
#define CHECK_BIG_ENDIAN \
    ({                   \
        int i = 1;       \
        !*((char *)&i);  \
    })

/* Get the min(max) one of the two numbers */
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Get the aligned value */
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

/* Get the aligned pointer size */
#define ALIGN_PTR(x) ALIGN(x, sizeof(uintptr_t))

/* Get the number of elements in an array */
#define COUNT_OF(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

/* pointer to integer */
#define PTR2INT(p) ((int)(intptr_t)(p))

/*
** Get the 'type' pointer from the pointer to `member`
** which is embedded inside the 'type'
*/
#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_COMMON_H_ */
