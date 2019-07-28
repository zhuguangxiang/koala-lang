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

struct codeinfo {
  struct vector locvec;
  Object *consts;
  int size;
  unsigned char insts[0];
};

typedef struct codeobject {
  OBJECT_HEAD
  TypeObject *type;
  TypeDesc *proto;
  struct codeinfo *code;
} CodeObject;

API_DATA(TypeObject) Code_Type;
Object *Code_From_CFunc(MethodDef *m);
void *Code_Get_CFunc(Object *self);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
