/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MAP_OBJECT_H_
#define _KOALA_MAP_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mapobject {
  OBJECT_HEAD
} MapObject;

extern Klass map_type;
void init_mapobject(void);
void fini_mapobject(void);
Object *new_map(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MAP_OBJECT_H_ */
