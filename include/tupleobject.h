/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TUPLE_OBJECT_H_
#define _KOALA_TUPLE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TupleObject {
    OBJECT_HEAD
    int start;
    int stop;
    GcArrayObject *array;
} TupleObject;

extern TypeObject tuple_type;
#define IS_TUPLE(ob) IS_TYPE((ob), &tuple_type)

#define TUPLE_ITEMS(x) (Value *)((((TupleObject *)(x))->array) + 1)
#define TUPLE_LEN(x)   (((TupleObject *)(x))->stop - ((TupleObject *)(x))->start)

Object *kl_new_tuple(int size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TUPLE_OBJECT_H_ */
