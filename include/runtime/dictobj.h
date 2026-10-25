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

extern TypeObject dict_keys_iter_type;
extern TypeObject dict_values_iter_type;
extern TypeObject dict_view_iter_type;
extern TypeObject dict_rev_iter_type;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DICT_OBJECT_H_ */
