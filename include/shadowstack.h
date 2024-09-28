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

/* trace shadow stack */
typedef struct _ShadowStack {
    struct _ShadowStack *back;
    const char *fname;
    int avail;
    int size;
    void *objs[0];
} ShadowStack;

/* clang-format off */

#define SHADOW_STACK(n) struct { \
    ShadowStack ss; \
    void *objs[n]; \
}

#define INIT_GC_STACK(_ks, n) \
    SHADOW_STACK(n) _stk = {{NULL, __FUNCTION__, 0, (n)}}; \
    do { \
        _stk.ss.back = _ks->shadow_stacks; \
        _ks->shadow_stacks = &_stk.ss; \
    } while (0)

#define gc_stack_push(obj) do { \
    ShadowStack *_ss = &_stk.ss; \
    ASSERT(_ss->avail < _ss->size); \
    _ss->objs[_ss->avail++] = (obj); \
} while (0)

/* remove trace stack from cfunc frame */
#define FINI_GC_STACK(_ks) do { \
    _ks->shadow_stacks = _stk.ss.back; \
} while (0)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SHADOW_STACK_H_ */
