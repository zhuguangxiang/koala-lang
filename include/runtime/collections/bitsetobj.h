/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BITSET_OBJECT_H_
#define _KOALA_BITSET_OBJECT_H_

#include "object.h"
#include "common/bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BitSetObject {
    OBJECT_HEAD
    BitSet bitset;
} BitSetObject;

extern TypeObject bitset_type;
#define IS_BITSET(ob) IS_TYPE((ob), &bitset_type)

Object *kl_new_bitset(int nbits);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BITSET_OBJECT_H_ */
