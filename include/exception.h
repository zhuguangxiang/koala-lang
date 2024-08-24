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

/* Koala does support catch exception.
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
void _raise_exc(KoalaState *ks, const char *fmt, ...);
#define _exc_occurred(ks) ((ks)->exc != NULL)

/* clang-format off */

#define raise_exc(fmt, ...) do {        \
    KoalaState *ks = __ks();            \
    _raise_exc(ks, fmt, ##__VA_ARGS__); \
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
