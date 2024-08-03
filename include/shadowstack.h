/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SHADOW_STACK_H_
#define _KOALA_SHADOW_STACK_H_

#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * gc shadow stack, defined in cfunc frame, not globals
 * roots[0] = num_roots,
 * roots[1] = previous root,
 * roots[2...n] = actual roots' objects
 */

/* clang-format off */

#define GC_STACK(nargs) \
    KoalaState *__gc_ks__ = __ks(); \
    int __gc_index__ = 2; \
    void *__gc_stk__[2 + nargs] = { (void *)nargs, __gc_ks__->gcroots, NULL }; \
    __gc_ks__->gcroots = __gc_stk__;

#define kl_gc_push(arg) \
    __gc_stk__[__gc_index__] = arg; \
    ++__gc_index__;

/* remove traced objects from cfunc frame */
#define kl_gc_pop() (__gc_ks__->gcroots = ((void **)__gc_ks__->gcroots)[1])

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SHADOW_STACK_H_ */
