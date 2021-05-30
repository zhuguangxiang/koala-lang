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

/* like julia gc */

extern void *gcroots;

/*
    roots[0] = num_roots,
    roots[1] = previous root,
    roots[2...n] = actual roots' objects
 */

/* clang-format off */

/* save traced objects in func frame */
#define gc_push1(arg1) \
    void *__gc_stkf[] = { (void *)1, gcroots, arg1 }; \
    gcroots = __gc_stkf;

#define gc_push2(arg1, arg2) \
    void *__gc_stkf[] = { (void *)2, gcroots, arg1, arg2 }; \
    gcroots = __gc_stkf;

#define gc_push3(arg1, arg2, args3) \
    void *__gc_stkf[] = { (void *)3, gcroots, arg1, arg2, arg3 }; \
    gcroots = __gc_stkf;

/* remove traced objects from func frame */
#define gc_pop() (gcroots = ((void **)gcroots)[1])

/* clang-format on */

/* object finalized func for close resource */
typedef void (*GcFiniFunc)(void *);

/* count of __VA_ARGS__ */
#define VA_NARGS(type, ...) \
    ((type)(sizeof((type[]){ __VA_ARGS__ }) / sizeof(type)))

/*
 objmap[0] = count,
 objmap[1...n] = offset,
*/

/* allocate object */
void *gc_alloc(int size, int *objmap, GcFiniFunc fini);

/* allocate array */
void *gc_alloc_array(int size, int isobj);

/* expand array */
void gc_expand_array(void **arr, int size);

/* start to gc */
void gc(void);

/* initialize gc */
void gc_init(void);

/* finalize gc */
void gc_fini(void);

/* gc mem stat */
void gc_stat(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_GC_H_ */
