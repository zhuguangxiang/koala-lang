/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_ARRAYOBJECT_H_
#define _KOALA_ARRAYOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

Object *array_new(uint32 tp_map);
void array_reserve(Object *self, int32 count);
void array_append(Object *self, uintptr val);
int32 array_length(Object *self);
void array_set(Object *self, uint32 index, uintptr val);
uintptr array_get(Object *self, uint32 index);
void array_print(Object *self);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAYOBJECT_H_ */
