/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include <inttypes.h>
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct codeinfo {
  struct vector locvec;
  Object *consts;
  int size;
  uint8_t insts[0];
};

#define CODE_KCODE 0
#define CODE_CFUNC 1

typedef struct codeobject {
  OBJECT_HEAD
  TypeDesc *proto;
  int kind;
  union {
    cfunc_t cfunc;
    struct codeinfo *code;
  };
} CodeObject;

extern Klass code_type;
Object *code_from_cfunc(struct cfuncdef *f);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
