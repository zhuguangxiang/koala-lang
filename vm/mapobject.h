/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_MAPOBJECT_H_
#define _KOALA_MAPOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

ObjectRef map_new(uint32 tp_map);
int32 map_put_absent(ObjectRef self, uintptr key, uintptr val);
void map_put(ObjectRef self, uintptr key, uintptr val, uintptr *old_val);
int32 map_get(ObjectRef self, uintptr key, uintptr *val);
int32 map_remove(ObjectRef self, uintptr key, uintptr *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MAPOBJECT_H_ */
