/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct codeobject {
  OBJECT_HEAD
  char *name;
  Object *owner;
  TypeDesc *proto;
  int locals;
  Vector locvec;
  Object *consts;
  int size;
  uint8_t codes[0];
} CodeObject;

extern TypeObject code_type;
#define Code_Check(ob) (OB_TYPE(ob) == &code_type)
Object *Code_New(char *name, TypeDesc *proto, int locals,
                 uint8_t *codes, int size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
