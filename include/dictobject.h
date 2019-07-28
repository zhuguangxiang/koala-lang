/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_DICT_OBJECT_H_
#define _KOALA_DICT_OBJECT_H_

#include "stringobject.h"
#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dictobject {
  OBJECT_HEAD
  TypeObject *ktype;
  TypeObject *vtype;
  struct hashmap map;
} DictObject;

extern TypeObject Dict_Type;
#define Dict_Check(ob) (OB_TYPE(ob) == &Dict_Type)
Object *Dict_New_WithTypes(TypeObject *ktype, TypeObject *vtype);
Object *Dict_Get(Object *self, Object *key);
int Dict_Put(Object *self, Object *key, Object *val);
static inline int Dict_Contains(Object *self, Object *key)
{
  Object *val = Dict_Get(self, key);
  return val ? 1 : 0;
}
static inline Object *Dict_New(void)
{
  return Dict_New_WithTypes(&String_Type, &Any_Type);
}


#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DICT_OBJECT_H_ */
