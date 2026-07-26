/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SLICE_OBJECT_H_
#define _KOALA_SLICE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SliceObject {
    OBJECT_HEAD
    TValue start;
    TValue end;
    TValue step;
} SliceObject;

extern TypeObject slice_type;
#define IS_SLICE(ob) IS_TYPE((ob), &slice_type)

Object *kl_new_slice(TValue *items);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SLICE_OBJECT_H_ */
