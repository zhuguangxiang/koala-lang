/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_HASHSET_OBJECT_H_
#define _KOALA_HASHSET_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HashSetObject {
    OBJECT_HEAD
    HashMap map;
} HashSetObject;

extern TypeObject hashset_type;
#define IS_HASHSET(ob) IS_TYPE((ob), &hashset_type)

Object *kl_new_hashset(void);

extern TypeObject hashset_iter_type;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_HASHSET_OBJECT_H_ */
