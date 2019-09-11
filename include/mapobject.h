/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MAP_OBJECT_H_
#define _KOALA_MAP_OBJECT_H_

#include "stringobject.h"
#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dictobject {
  OBJECT_HEAD
  TypeDesc *ktype;
  TypeDesc *vtype;
  HashMap map;
} MapObject;

extern TypeObject map_type;
#define map_check(ob) (OB_TYPE(ob) == &map_type)
void init_map_type(void);
Object *map_new(TypeDesc *ktype, TypeDesc *vtype);
Object *map_get(Object *self, Object *key);
int map_put(Object *self, Object *key, Object *val);
static inline int dict_contains(Object *self, Object *key)
{
  Object *val = map_get(self, key);
  return val ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MAP_OBJECT_H_ */
