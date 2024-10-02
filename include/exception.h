/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_EXCEPTION_H_
#define _KOALA_EXCEPTION_H_

#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Koala does NOT support catch exception.
 * If an exception occurred, it must be fixed.
 */

typedef struct _TraceBack {
    struct _TraceBack *back;
    const char *file;
    int lineno;
} TraceBack;

typedef struct _Exception {
    OBJECT_HEAD
    const char *msg;
    TraceBack *back;
} Exception;

extern TypeObject exc_type;
void _raise_exc_fmt(KoalaState *ks, const char *fmt, ...);
void _raise_exc_str(KoalaState *ks, const char *str);
#define _exc_occurred(ks) ((ks)->exc != NULL)
void kl_trace_here(CallFrame *cf);

void _print_exc(KoalaState *ks);

/* clang-format off */

#define print_exc() do {     \
    KoalaState *ks = __ks(); \
    _print_exc(ks);          \
} while (0)

#define raise_exc_fmt(fmt, args...) do {        \
    KoalaState *ks = __ks();                \
    _raise_exc_fmt(ks, fmt, args); \
} while(0)

#define raise_exc_str(str) do { \
    KoalaState *ks = __ks();    \
    _raise_exc_str(ks, str);    \
} while(0)

#define exc_occurred() ({       \
    KoalaState *ks = __ks();    \
    _exc_occurred(ks);          \
})

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EXCEPTION_H_ */
