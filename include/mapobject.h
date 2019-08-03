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

typedef struct mapobject {
  OBJECT_HEAD
  TypeObject *ktype;
  TypeObject *vtype;
  struct hashmap map;
} MapObject;

extern TypeObject Map_Type;
#define Map_Check(ob) (OB_TYPE(ob) == &Map_Type)
Object *Map_New_Types(TypeObject *ktype, TypeObject *vtype);
Object *Map_Get(Object *self, Object *key);
int Map_Put(Object *self, Object *key, Object *val);
static inline int Map_Contains(Object *self, Object *key)
{
  Object *val = Map_Get(self, key);
  return val ? 1 : 0;
}
static inline Object *Map_New(void)
{
  return Map_New_Types(&String_Type, &Any_Type);
}


#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MAP_OBJECT_H_ */
