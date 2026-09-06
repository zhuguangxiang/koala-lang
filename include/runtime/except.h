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

void _print_exc(KoalaState *ks);

#define print_exc() \
    do { \
        KoalaState *ks = __ks(); \
        _print_exc(ks); \
    } while (0)

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

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EXCEPT_H_ */
