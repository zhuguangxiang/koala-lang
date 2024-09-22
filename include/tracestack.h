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
    const char *fname;
    Vector gc_stk;
} TraceStack;

/* clang-format off */

#define INIT_TRACE_STACK() \
    KoalaState *__trace_ks = __ks(); \
    TraceStack __trace_stk = {__FUNCTION__, VECTOR_INIT_PTR}; \
    TraceStack *__trace_stk_p = &__trace_stk; \
    vector_push_back(&__trace_ks->trace_stacks, &__trace_stk_p)

#define TRACE_STACK_PUSH(obj) vector_push_back(&__trace_stk.gc_stk, (obj))

/* remove traced objects from cfunc frame */
#define FINI_TRACE_STACK() \
    vector_pop_back(&__trace_ks->trace_stacks, NULL); \
    vector_fini(&__trace_stk.gc_stk)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TRACE_STACK_H_ */
