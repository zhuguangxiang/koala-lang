/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_DICT_OBJECT_H_
#define _KOALA_DICT_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DictObject {
    OBJECT_HEAD
    HashMap map;
} DictObject;

extern TypeObject dict_type;
#define IS_DICT(ob) IS_TYPE((ob), &dict_type)

Object *kl_new_dict(void);

/* Insert or replace a (key, value) mapping. Used by the module loader to
 * materialize constant dict literals, mirroring how lists are built. */
void kl_dict_add(Object *dict, TValue *key, TValue *value);

/* Call `visit(key, value, arg)` for each mapping, in insertion order.
 * `visit` must not change the dict. */
typedef void (*DictVisitFunc)(TValue *key, TValue *value, void *arg);
void kl_dict_foreach(Object *dict, DictVisitFunc visit, void *arg);

extern TypeObject dict_keys_iter_type;
extern TypeObject dict_values_iter_type;
extern TypeObject dict_view_iter_type;
extern TypeObject dict_rev_iter_type;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DICT_OBJECT_H_ */
