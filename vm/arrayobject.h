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

ObjectRef array_new(int32 itemsize, int8 ref);
void array_append(ObjectRef self, uintptr_t val);
int32 array_length(ObjectRef self);
void array___set_item__(ObjectRef self, uint32 index, uintptr_t val);
uintptr_t array___get_item__(ObjectRef self, uint32 index);
void array_print(ObjectRef self);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ARRAYOBJECT_H_ */
