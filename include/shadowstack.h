/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SHADOW_STACK_H_
#define _KOALA_SHADOW_STACK_H_

#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/* trace shadow stack */
typedef struct _ShadowStack {
    struct _ShadowStack *back;
    const char *fname;
    uint32_t count;
    void **objs;
} ShadowStack;

/* clang-format off */

#define kl_gc_protect(...) \
    void *_objs_##__LINE__[] = { __VA_ARGS__ }; \
    ShadowStack ss_##__LINE__ = { \
        .back = __ks()->shadow_stacks, \
        .fname = __FUNCTION__, \
        .count = sizeof(_objs_##__LINE__) / sizeof(void *), \
        .objs = _objs_##__LINE__, \
    }; \
    __attribute__((cleanup(_kl_gc_ss_pop))) \
    ShadowStack *_ss_##__LINE__ = &ss_##__LINE__; \
    __ks()->shadow_stacks = _ss_##__LINE__;

/* clang-format on */

static inline void _kl_gc_ss_pop(ShadowStack **ss)
{
    KoalaState *ks = __ks();
    ASSERT(ks->shadow_stacks == *ss);
    ks->shadow_stacks = (*ss)->back;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SHADOW_STACK_H_ */
