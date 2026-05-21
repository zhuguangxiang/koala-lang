/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LIST_OBJECT_H_
#define _KOALA_LIST_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ListObject {
    OBJECT_HEAD
    size_t start;
    size_t end;
    size_t capacity;
    TValue *array;
} ListObject;

extern TypeObject list_type;
#define IS_LIST(ob) IS_TYPE((ob), &list_type)

Object *kl_new_list(TValue *items, int size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_OBJECT_H_ */
