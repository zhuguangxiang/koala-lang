/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_EXCEPT_H_
#define _KOALA_EXCEPT_H_

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline Object *get_exc()
{
    KoalaState *ks = __ks();
    Object *exc = ks->exc;
    ASSERT(exc);
    ks->exc = NULL;
    return exc;
}

void print_exc(Object *obj);

void _raise_exc_fmt(KoalaState *ks, char *fmt, ...);

#define raise_exc_fmt(fmt, args...) \
    do { \
        KoalaState *ks = __ks(); \
        _raise_exc_fmt(ks, fmt, args); \
    } while (0)

void _raise_exc_str(KoalaState *ks, char *str);

#define raise_exc_str(str) \
    do { \
        KoalaState *ks = __ks(); \
        _raise_exc_str(ks, str); \
    } while (0)

void trace_here(CallFrame *cf);

void exc_free(Object *obj);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EXCEPT_H_ */
