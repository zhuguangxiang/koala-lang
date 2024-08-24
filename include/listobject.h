/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_LIST_OBJECT_H_
#define _KOALA_LIST_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ListObject {
    OBJECT_HEAD
    ssize_t start;
    ssize_t end;
    char *array;
} ListObject;

extern TypeObject list_type;
#define IS_LIST(ob) IS_TYPE((ob), &list_type)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_OBJECT_H_ */
