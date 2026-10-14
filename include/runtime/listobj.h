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

Object *kl_new_list(void);
Object *kl_list_from_array(TValue *items, int size);
void kl_list_append(Object *ob, TValue item);
void kl_list_prepend(Object *ob, TValue item);

static inline int kl_list_len(Object *ob)
{
    if (ob == NULL) return 0;

    ASSERT(IS_LIST(ob));
    ListObject *list = (ListObject *)ob;
    return (int)(list->end - list->start);
}

static inline TValue *kl_list_get(Object *ob, int index)
{
    ASSERT(IS_LIST(ob));
    ListObject *list = (ListObject *)ob;
    ASSERT(index >= 0 && index < (int)(list->end - list->start));
    return &list->array[list->start + index];
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_OBJECT_H_ */
