/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_RANGE_OBJECT_H_
#define _KOALA_RANGE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RangeObject {
    OBJECT_HEAD
    TValue start;
    TValue stop;
    TValue step;
} RangeObject;

extern TypeObject range_type;
#define IS_RANGE(ob) IS_TYPE((ob), &range_type)

Object *kl_new_range(TValue *items, int count);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_RANGE_OBJECT_H_ */
