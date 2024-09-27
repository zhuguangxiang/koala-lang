/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_FORMATTER_OBJECT_H_
#define _KOALA_FORMATTER_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FormatterObject {
    OBJECT_HEAD
    uint32_t flags;
    char fill;
    char align;
    int width;
    int precision;
    // io.Writer
    Object *buf;
} FormatterObject;

extern TypeObject tuple_type;
#define IS_TUPLE(ob) IS_TYPE((ob), &tuple_type)

#define TUPLE_ITEMS(x) (((TupleObject *)(x))->values)
#define TUPLE_SIZE(x)  (((TupleObject *)(x))->size)

Object *kl_new_tuple(int size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FORMATTER_OBJECT_H_ */
