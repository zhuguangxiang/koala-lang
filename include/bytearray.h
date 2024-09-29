/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BYTEARRAY_OBJECT_H_
#define _KOALA_BYTEARRAY_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ByteArrayObject {
    OBJECT_HEAD
    int cap;
    int len;
    GcArrayObject *buf;
} ByteArrayObject;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BYTEARRAY_OBJECT_H_ */
