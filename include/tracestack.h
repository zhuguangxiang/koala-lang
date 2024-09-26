/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TRACE_STACK_H_
#define _KOALA_TRACE_STACK_H_

#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/* trace shadow stack */
typedef struct _TraceStack {
    struct _TraceStack *back;
    const char *fname;
    int avail;
    int size;
    void **obj_p[0];
} TraceStack;

/* clang-format off */

#define TRACE_STACK(n) struct { \
    TraceStack trace; \
    void **obj_p[n]; \
}

#define INIT_TRACE_STACK(n) \
    KoalaState *_ks = __ks(); \
    TRACE_STACK(n) _stk = {{NULL, __FUNCTION__, 0, (n)}}; \
    do { \
        _stk.trace.back = _ks->trace_stacks; \
        _ks->trace_stacks = &_stk.trace; \
    } while (0)

#define TRACE_STACK_PUSH(obj) do { \
    TraceStack *_ts = &_stk.trace; \
    ASSERT(_ts->avail < _ts->size); \
    _ts->obj_p[_ts->avail++] = (void **)(obj); \
} while (0)

/* remove trace stack from cfunc frame */
#define FINI_TRACE_STACK() do { \
    _ks->trace_stacks = _stk.trace.back; \
} while (0)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TRACE_STACK_H_ */
