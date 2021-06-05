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

// clang-format off

#define GC_STACK(nargs) \
    void *__gc_stkf[nargs + 2] = { (void *)nargs, gcroots }; \
    gcroots = __gc_stkf;

// clang-format on

#define gc_push(arg, idx) __gc_stkf[2 + idx] = arg

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
