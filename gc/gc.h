/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_GC_H_
#define _KOALA_GC_H_

#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: per-thread? */
extern void *gcroots;

/*
 roots[0] = num_roots,
 roots[1] = previous root,
 roots[2...n] = actual roots' objects
 */

#define GC_STACK(nargs)                               \
    void *__gc_stkf__[nargs + 2] = { NULL, gcroots }; \
    gcroots = __gc_stkf__;

#define gc_push1(arg1)              \
    do {                            \
        __gc_stkf__[0] = (void *)1; \
        __gc_stkf__[2] = arg1;      \
    } while (0)

#define gc_push2(arg1, arg2)        \
    do {                            \
        __gc_stkf__[0] = (void *)2; \
        __gc_stkf__[2] = arg1;      \
        __gc_stkf__[3] = arg2;      \
    } while (0)

#define gc_push3(arg1, arg2, arg3)  \
    do {                            \
        __gc_stkf__[0] = (void *)3; \
        __gc_stkf__[2] = arg1;      \
        __gc_stkf__[3] = arg2;      \
        __gc_stkf__[4] = arg3;      \
    } while (0)

#define gc_push4(arg1, arg2, arg3, arg4) \
    do {                                 \
        __gc_stkf__[0] = (void *)4;      \
        __gc_stkf__[2] = arg1;           \
        __gc_stkf__[3] = arg2;           \
        __gc_stkf__[4] = arg3;           \
        __gc_stkf__[5] = arg4;           \
    } while (0)

/* remove traced objects from func frame */
#define gc_pop() (gcroots = ((void **)gcroots)[1])

/*
 objmap[0] = count,
 objmap[1...n] = offset,
*/

/* allocate object */
void *gc_alloc(int size, int *objmap);

/* allocate array */
void *gc_alloc_array(int num, int size, int isobj);

/* start to gc */
void gc(void);

/* initialize gc */
void gc_init(int size);

/* finalize gc */
void gc_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
