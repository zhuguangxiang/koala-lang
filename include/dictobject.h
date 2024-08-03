/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_DICT_OBJECT_H_
#define _KOALA_DICT_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DictObject {
    OBJECT_HEAD
    HashMapEntry entry;
    const char *key;
    Value value;
} DictObject;

extern TypeObject dict_type;
#define IS_DICT(ob) IS_TYPE((ob), &dict_type)

Object *kl_new_dict(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DICT_OBJECT_H_ */
