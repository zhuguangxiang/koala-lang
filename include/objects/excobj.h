/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_EXCOBJ_H_
#define _KOALA_EXCOBJ_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Koala does NOT support catch exception.
 * If an exception occurred, it must be fixed.
 */

typedef struct _TraceBack {
    struct _TraceBack *back;
    char *file;
    int lineno;
} TraceBack;

typedef struct _Exception {
    OBJECT_HEAD
    char *msg;
    TraceBack *back;
} Exception;

extern TypeObject exc_type;

Object *kl_new_exc(char *msg);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EXCOBJ_H_ */
