/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SHADOW_STACK_H_
#define _KOALA_SHADOW_STACK_H_

#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/* gc shadow stack */

/* clang-format off */

#define INIT_GC_STACK() \
    KoalaState *__gc_ks = __ks(); \
    Vector *__gc_stk = vector_create_ptr(); \
    vector_push_back(&__gc_ks->gcroots, &__gc_stk)

#define GC_STACK_PUSH(obj) vector_push_back(__gc_stk, (obj))

/* remove traced objects from cfunc frame */
#define FINI_GC_STACK() \
    vector_pop_back(&__gc_ks->gcroots, NULL); \
    vector_destroy(__gc_stk)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SHADOW_STACK_H_ */
