/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TUPLE_OBJECT_H_
#define _KOALA_TUPLE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TupleObject {
    OBJECT_HEAD
    size_t size;
    TValue array[0];
} TupleObject;

extern TypeObject tuple_type;
#define IS_TUPLE(ob) IS_TYPE((ob), &tuple_type)

#define TUPLE_ITEMS(x) (((TupleObject *)(x))->array)
#define TUPLE_SIZE(x)  (((TupleObject *)(x))->size)

Object *kl_new_tuple(TValue *items, int count);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TUPLE_OBJECT_H_ */
